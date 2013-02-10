
#include <string.h>
#include <stdlib.h>

#include "fcgi_handler.h"

#define MULTIPART_FORM_DATA_STRING              "multipart/form-data"
#define BOUNDARY_STRING                         "boundary="
#define CONTENT_DISPOSITION_STRING              "Content-Disposition:"
#define CONTENT_TYPE_STRING                     "Content-Type:"
#define FORM_DATA_STRING                        "form-data"
#define ATTACHMENT_STRING                       "attachment"
#define FIELDNAME_STRING                        "name=\""

namespace fp {

class formdata_parser {
private:
    typedef enum {
        upload_state_boundary_seek,
        upload_state_after_boundary,
        upload_state_headers,
        upload_state_data,
        upload_state_finish
    } formdata_parser_state_t;

public:
    formdata_parser(const std::string &_content_type, size_t max_header_len, size_t buffer_size, std::map<const std::string, std::string> &_formdata_params);
    ~formdata_parser();

    void upload_parse_part_header(char *header, char *header_end);
    void upload_parse_content_type(const std::string &content_type);
    void upload_discard_part_attributes();
    void upload_start_part();
    void upload_finish_part();
    void upload_abort_part();
    void upload_flush_output_buffer();

    void upload_init_ctx();
    void upload_shutdown_ctx();

    void upload_putc(u_char c);

    void upload_process_buf(u_char *start, u_char *end);

private:
    std::map<const std::string, std::string> &formdata_params;

    std::string                 boundary;
    std::string                 field_name;
    std::string                 field_value;
    std::string                 content_type;

    formdata_parser_state_t     state;

    std::string::iterator       boundary_start;
    std::string::iterator       boundary_pos;
    
    u_char                      *header_accumulator;
    u_char                      *header_accumulator_end;
    u_char                      *header_accumulator_pos;

    u_char                      *output_buffer;
    u_char                      *output_buffer_end;
    u_char                      *output_buffer_pos;

    unsigned int                discard_data:1;
    unsigned int                first_part:1;
};

formdata_parser::formdata_parser(const std::string &_content_type, size_t max_header_len, size_t buffer_size, std::map<const std::string, std::string> &_formdata_params)
    : formdata_params(_formdata_params)
    , header_accumulator(new u_char[max_header_len + 1])
    , output_buffer(new u_char[buffer_size])
    , first_part(1)
{
	if(header_accumulator == NULL) {
        throw fcgi_exception("insufficient memory", HTTP_INTERNAL_SERVER_ERROR);
    }

	header_accumulator_pos = header_accumulator;
	header_accumulator_end = header_accumulator + max_header_len;

	if(output_buffer == NULL) {
        throw fcgi_exception("insufficient memory", HTTP_INTERNAL_SERVER_ERROR);
    }

    output_buffer_pos = output_buffer;
    output_buffer_end = output_buffer + buffer_size;

    upload_init_ctx();

    upload_parse_content_type(_content_type);
}

formdata_parser::~formdata_parser() {
    delete [] header_accumulator;
    delete [] output_buffer;
}

void formdata_parser::upload_parse_part_header(char *header, char *header_end) { /* {{{ */
    if(!strncasecmp(CONTENT_DISPOSITION_STRING, header, sizeof(CONTENT_DISPOSITION_STRING) - 1)) {
        char *p = header + sizeof(CONTENT_DISPOSITION_STRING) - 1;
        char *fieldname_start, *fieldname_end;

        p += strspn(p, " ");
        
        if(strncasecmp(FORM_DATA_STRING, p, sizeof(FORM_DATA_STRING)-1) && 
                strncasecmp(ATTACHMENT_STRING, p, sizeof(ATTACHMENT_STRING)-1)) {
            throw fcgi_exception("Content-Disposition is not form-data or attachment", HTTP_BAD_REQUEST);
        }

        fieldname_start = strstr(p, FIELDNAME_STRING);

        if(fieldname_start != 0) {
            fieldname_start += sizeof(FIELDNAME_STRING)-1;

            fieldname_end = fieldname_start + strcspn(fieldname_start, "\"");

            if(*fieldname_end != '\"') {
                throw fcgi_exception("malformed fieldname in part header", HTTP_BAD_REQUEST);
            }

            field_name.assign(fieldname_start, fieldname_end - fieldname_start);
        }
    }else if(!strncasecmp(CONTENT_TYPE_STRING, header, sizeof(CONTENT_TYPE_STRING)-1)) {
        char *content_type_str = header + sizeof(CONTENT_TYPE_STRING)-1;
        
        content_type_str += strspn(content_type_str, " ");

        content_type.assign(content_type_str, header_end - content_type_str);

        if(content_type.empty()) {
            throw fcgi_exception("empty Content-Type in part header", HTTP_BAD_REQUEST);
        }
    }
} /* }}} */

void formdata_parser::upload_discard_part_attributes() { /* {{{ */
    field_name.clear();
    field_value.clear();
    content_type.clear();
} /* }}} */

void formdata_parser::upload_start_part() { /* {{{ */
} /* }}} */

void formdata_parser::upload_finish_part() { /* {{{ */

    formdata_params.insert( std::make_pair(field_name, field_value) );

    upload_discard_part_attributes();

    discard_data = 0;
} /* }}} */

void formdata_parser::upload_abort_part() { /* {{{ */

    upload_discard_part_attributes();

    discard_data = 0;
} /* }}} */

void formdata_parser::upload_flush_output_buffer() { /* {{{ */
    if(output_buffer_pos > output_buffer) {
        if(field_value.empty())
            field_value.assign(output_buffer, output_buffer_pos);
        output_buffer_pos = output_buffer;	
    }
} /* }}} */

void formdata_parser::upload_init_ctx() { /* {{{ */
	state = upload_state_boundary_seek;

    field_name.clear();
    field_value.clear();
    content_type.clear();

    discard_data = 0;
} /* }}} */

void formdata_parser::upload_shutdown_ctx() { /* {{{ */
    // Abort file if we still processing it
    if(state == upload_state_data) {
        upload_flush_output_buffer();
        upload_abort_part();
    }

    upload_discard_part_attributes();
} /* }}} */

void formdata_parser::upload_parse_content_type(const std::string &content_type) { /* {{{ */
    // Find colon in content type string, which terminates mime type
    const char *mime_type_end_ptr = strchr(content_type.c_str(), ';');
    const char *boundary_start_ptr, *boundary_end_ptr;
    size_t boundary_start_pos;

    if(mime_type_end_ptr == NULL) {
        throw fcgi_exception("no boundary found in Content-Type", HTTP_BAD_REQUEST);
    }

    if(strncasecmp(content_type.c_str(), MULTIPART_FORM_DATA_STRING,
        sizeof(MULTIPART_FORM_DATA_STRING) - 1))
    {
        throw fcgi_exception("Content-Type is not multipart/form-data", HTTP_UNSUPPORTED_MEDIA_TYPE);
    }

    boundary_start_pos = content_type.find(BOUNDARY_STRING, mime_type_end_ptr - content_type.c_str());

    boundary_start_ptr = content_type.c_str() + boundary_start_pos;

    if(boundary_start_ptr == NULL) {
        throw fcgi_exception("no boundary found in Content-Type", HTTP_BAD_REQUEST);
    }

    boundary_start_ptr += sizeof(BOUNDARY_STRING) - 1;
    boundary_end_ptr = boundary_start_ptr + strcspn((char*)boundary_start_ptr, " ;\n\r");

    if(boundary_end_ptr == boundary_start_ptr) {
        throw fcgi_exception("boundary is empty", HTTP_BAD_REQUEST);
    }

    boundary.assign(boundary_start_ptr, boundary_end_ptr);
    
    // Prepend boundary data by \r\n--
    boundary.insert(boundary.begin(), '-'); 
    boundary.insert(boundary.begin(), '-'); 
    boundary.insert(boundary.begin(), '\n'); 
    boundary.insert(boundary.begin(), '\r'); 

    /*
     * NOTE: first boundary doesn't start with \r\n. Here we
     * advance 2 positions forward. We will return 2 positions back 
     * later
     */
    boundary_start = boundary.begin() + 2;
    boundary_pos = boundary_start;
} /* }}} */

void formdata_parser::upload_putc(u_char c) { /* {{{ */
    if(!discard_data) {
        *output_buffer_pos = c;

        output_buffer_pos++;

        if(output_buffer_pos == output_buffer_end)
            upload_flush_output_buffer();	
    }
} /* }}} */

void formdata_parser::upload_process_buf(u_char *start, u_char *end) { /* {{{ */
	u_char *p;

	// No more data?
	if(start == end) {
		if(state != upload_state_finish)
            throw fcgi_exception("malformed body", HTTP_BAD_REQUEST);
		else
			return; // Otherwise confirm end of stream
    }

	for(p = start; p != end; p++) {
		switch(state) {
			/*
			 * Seek the boundary
			 */
			case upload_state_boundary_seek:
				if(*p == *boundary_pos) 
					boundary_pos++;
				else
					boundary_pos = boundary_start;

				if(boundary_pos == boundary.end()) {
					state = upload_state_after_boundary;
					boundary_start = boundary.begin();
					boundary_pos = boundary_start;
				}
				break;
			case upload_state_after_boundary:
				switch(*p) {
					case '\n':
						state = upload_state_headers;
                        header_accumulator_pos = header_accumulator;
					case '\r':
						break;
					case '-':
						state = upload_state_finish;
						break;
				}
				break;
			/*
			 * Collect and store headers
			 */
			case upload_state_headers:
				switch(*p) {
					case '\n':
						if(header_accumulator_pos == header_accumulator) {
                            //is_file = (file_name.data == 0) || (file_name.len == 0) ? 0 : 1;

                            upload_start_part();
                            
                            state = upload_state_data;
                            output_buffer_pos = output_buffer;	
                        } else {
                            *header_accumulator_pos = '\0';

                            try{
                                upload_parse_part_header((char*)header_accumulator, (char*)header_accumulator_pos);

                                header_accumulator_pos = header_accumulator;
                            } catch(std::exception &_e) {
                                state = upload_state_finish;
                                throw;
                            }
                        }
					case '\r':
						break;
					default:
						if(header_accumulator_pos < header_accumulator_end - 1)
							*header_accumulator_pos++ = *p;
						else {
                            state = upload_state_finish;
                            throw fcgi_exception("header is too long", HTTP_BAD_REQUEST);
                        }
						break;
				}
				break;
			/*
			 * Search for separating or terminating boundary
			 * and output data simultaneously
			 */
			case upload_state_data:
				if(*p == *boundary_pos) 
					boundary_pos++;
				else {
					if(boundary_pos == boundary_start) {
                        // IE 5.0 bug workaround
                        if(*p == '\n') {
                            /*
                             * Set current matched position beyond LF and prevent outputting
                             * CR in case of unsuccessful match by altering boundary_start 
                             */ 
                            boundary_pos = boundary.begin() + 2;
                            boundary_start = boundary.begin() + 1;
                        } else
                            upload_putc(*p);
                    } else {
						// Output partially matched lump of boundary
						std::string::iterator q;
						for(q = boundary_start; q != boundary_pos; q++)
							upload_putc(*q);

                        p--; // Repeat reading last character

						// And reset matched position
                        boundary_start = boundary.begin();
						boundary_pos = boundary_start;
					}
				}

				if(boundary_pos == boundary.end()) {
					state = upload_state_after_boundary;
					boundary_pos = boundary_start;

                    upload_flush_output_buffer();
                    if(!discard_data)
                        upload_finish_part();
                    else
                        upload_abort_part();
				}
				break;
			/*
			 * Skip trailing garbage
			 */
			case upload_state_finish:
				break;
		}
	}
} /* }}} */

#define STDIN_MAX 32768

void fcgi_request::parse_form_data() {
    std::string content_type, content_length;
    u_char *buf;
    unsigned long clen = 128 * 1024;

    content_type = get_fcgi_param("CONTENT_TYPE");
    content_length = get_fcgi_param("CONTENT_LENGTH");

    if(content_type.empty())
        throw std::logic_error("HTTP server is not configured to pass CONTENT_TYPE variable");

    if(!content_length.empty())
    {
        clen = strtol(content_length.c_str(), NULL, 10);

        if(clen == 0) {
            throw std::runtime_error("can't parse \"CONTENT_LENGTH\n");
        }

        // *always* put a cap on the amount of data that will be read
        if (clen > STDIN_MAX) clen = STDIN_MAX;

        formdata_parser p(content_type, 512, 128 * 1024, m_formdata_params);

        buf = new u_char[clen];

        while(!fcgi_in.eof() && !fcgi_in.fail()) {
            fcgi_in.read((char*)buf, clen);
            clen = fcgi_in.gcount();

            if(clen > 0) {
                try{
                    p.upload_process_buf(buf, buf + clen);
                } catch(...) {
                }
            }
        }

        delete [] buf;
    }
    else {
        throw std::logic_error("HTTP server is not configured to pass CONTENT_LENGTH variable");
    }

    do fcgi_in.ignore(1024); while (fcgi_in.gcount() == 1024);
}

void fcgi_request::parse_query_args()
{
    std::string query_args = get_fcgi_param("QUERY_STRING");

    size_t first, last, delim;

    first = 0;
    last = query_args.find_first_of('&');

    while(last != std::string::npos) {
        std::string arg(query_args, first, last - first);

        delim = arg.find_first_of('=');

        if(delim != std::string::npos) {
            std::string name(arg, 0, delim);
            std::string value(arg, delim + 1, last - first - delim);

            m_params.insert(std::make_pair(name, value));
        }
        else if(!arg.empty()) {
            m_params.insert(std::make_pair(arg, std::string()));
        }

        first = last + 1;

        last = query_args.find_first_of('c', first);
    }

    std::string arg(query_args, first);

    delim = arg.find_first_of('=');

    if(delim != std::string::npos) {
        std::string name(arg, 0, delim);
        std::string value(arg, delim + 1, last - first - delim);

        m_params.insert(std::make_pair(name, value));
    }
    else if(!arg.empty()) {
        m_params.insert(std::make_pair(arg, std::string()));
    }
}

};
