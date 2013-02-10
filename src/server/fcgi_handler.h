
#ifndef _HANDLER_H_
#define _HANDLER_H_

#include <stdexcept>
#include <map>

#include <netinet/in.h>

#include <fcgio.h>

namespace fp {

static const int HTTP_BAD_REQUEST                       = 400;
static const int HTTP_NOT_FOUND                         = 404;
static const int HTTP_UNSUPPORTED_MEDIA_TYPE            = 415;
static const int HTTP_INTERNAL_SERVER_ERROR             = 503;

class fcgi_exception : public std::runtime_error {
public:
    fcgi_exception(const char *_what, int _status)
        : std::runtime_error(_what)
        , m_status(_status)
    {
    }

    int status() const { return m_status; }

private:
    int         m_status;
};

/*
 * FastCGI request
 */
class fcgi_request {
public:
    fcgi_request(const FCGX_Request &_request)
        : fcgi_in(_request.in)
        , m_params()
        , m_formdata_params()
        , envp(_request.envp)
    {
        parse_query_args();
    }

    ~fcgi_request()
    {
    }

    fcgi_istream fcgi_in;

    std::string get_script_name() const { return get_fcgi_param("SCRIPT_NAME"); }

    bool has_param(const std::string &_name) const {
        std::map<std::string, std::string>::const_iterator i;
        i = m_params.find(_name);

        return i != m_params.end();
    }

    std::string get_param(const std::string &_name) const {
        std::map<std::string, std::string>::const_iterator i;
        i = m_params.find(_name);

        if(i == m_params.end()) {
            return std::string("");
        }

        return i->second;
    }

    std::string get_fcgi_param(const std::string &_name) const {
        char *c_param = FCGX_GetParam(_name.c_str(), envp);
        return c_param != NULL ? std::string(c_param) : std::string("");
    }

    bool has_formdata_param(const std::string &_name) const {
        std::map<std::string, std::string>::const_iterator i;
        i = m_formdata_params.find(_name);

        return i != m_formdata_params.end();
    }

    std::string get_formdata_param(const std::string &_name) const {
        std::map<std::string, std::string>::const_iterator i;
        i = m_formdata_params.find(_name);

        if(i == m_formdata_params.end()) {
            return std::string("");
        }

        return i->second;
    }

    void parse_form_data();

private:
    void parse_query_args();

private:
    std::map<const std::string, std::string> m_params;
    std::map<const std::string, std::string> m_formdata_params;
    char **envp;
};

/*
 * FastCGI response
 */
class fcgi_response {
public:
    fcgi_response(const FCGX_Request &_request)
        : fcgi_out(_request.out) 
        , fcgi_err(_request.err)
    {
    }
    
    fcgi_ostream fcgi_out;
    fcgi_ostream fcgi_err;
};

/*
 * fcgi handler 
 */
class fcgi_handler {
public:
    /*
     * Destroy fcgi handler
     */
    virtual ~fcgi_handler() {};

    /*
     * Initialize fcgi handler
     */
    virtual void init() = 0;

    /*
     * Shutdown fcgi handler
     */
    virtual void shutdown() = 0;

    /*
     * Handle request 
     *
     * @param _request  
     * @param _response
     */
    virtual void handle(fcgi_request &_request, fcgi_response &_response) = 0;
};

};

#endif //_HANDLER_H_
