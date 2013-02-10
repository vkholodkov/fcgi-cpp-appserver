
#include <memory>

#include "json.h"

JSONObject::~JSONObject() {
    for(std::map<std::string, struct JSONValue>::const_iterator i = m_items.begin() ; 
        i != m_items.end() ; i++)
    {
        switch(i->second.type) {
            case JSON_OBJECT:
                delete i->second.d.object;
                break;
            case JSON_ARRAY:
                delete i->second.d.array;
                break;
            case JSON_STRING:
                delete [] i->second.d.str;
                break;
            default:
                break;
        }
    }
}

JSONArray::~JSONArray() {
    for(std::vector<struct JSONValue>::const_iterator i = m_items.begin() ; 
        i != m_items.end() ; i++)
    {
        switch(i->type) {
            case JSON_OBJECT:
                delete i->d.object;
                break;
            case JSON_ARRAY:
                delete i->d.array;
                break;
            case JSON_STRING:
                delete [] i->d.str;
                break;
            default:
                break;
        }
    }
}

JSONArray *JSONObject::add_array(const std::string &key) {
    std::auto_ptr<JSONArray> array(new JSONArray());

    this->add(key, array.get());

    return array.release();
}

JSONObject *JSONObject::add_object(const std::string &key) {
    std::auto_ptr<JSONObject> object(new JSONObject());

    this->add(key, object.get());

    return object.release();
}

JSONArray *JSONArray::add_array() {
    std::auto_ptr<JSONArray> array(new JSONArray());

    this->add(array.get());

    return array.release();
}

JSONObject *JSONArray::add_object() {
    std::auto_ptr<JSONObject> object(new JSONObject());

    this->add(object.get());

    return object.release();
}

template <class InputIterator>
void serialize_string(std::ostream& o, InputIterator first, InputIterator last) {
    o << '"';

    while(first != last) {
        switch(*first) {
            case '"':
                o << "\\\"";
                break;
            case '\\':
                o << "\\\\";
                break;
            case '/':
                o << "\\/";
                break;
            case '\b':
                o << "\\b";
                break;
            case '\f':
                o << "\\f";
                break;
            case '\n':
                o << "\\n";
                break;
            case '\r':
                o << "\\r";
                break;
            case '\t':
                o << "\\t";
                break;
            default:
                o << *first;
                break;
        }

        first++;
    }

    o << '"';
}

std::ostream& operator<<(std::ostream& o, const JSONObject &object) {
    bool first = true;

    o << '{';

    for(std::map<std::string, struct JSONValue>::const_iterator i = object.m_items.begin() ; 
        i != object.m_items.end() ; i++)
    {
        if(!first) {
            o << ',';
        }
        else {
            first = false;
        }

        serialize_string(o, i->first.begin(), i->first.end());

        o << ':';

        switch(i->second.type) {
            case JSON_OBJECT:
                o << i->second.d.object;
                break;
            case JSON_ARRAY:
                o << i->second.d.array;
                break;
            case JSON_STRING:
                {
                    size_t *len = reinterpret_cast<size_t*>(i->second.d.str);
                    char *begin = reinterpret_cast<char*>(len+1);
                    char *end = begin + *len;

                    serialize_string(o, begin, end);
                }
                break;
            case JSON_INTEGER:
                o << i->second.d.i;
                break;
            case JSON_FLOAT:
                o << i->second.d.d;
                break;
        }
    }

    o << '}';

    return o;
}

std::ostream& operator<<(std::ostream& o, const JSONObject *object) {
    o << *object;
    return o;
}

std::ostream& operator<<(std::ostream& o, const JSONArray &array) {
    bool first = true;

    o << '[';

    for(std::vector<struct JSONValue>::const_iterator i = array.m_items.begin() ; 
        i != array.m_items.end() ; i++)
    {
        if(!first) {
            o << ',';
        }
        else {
            first = false;
        }

        switch(i->type) {
            case JSON_OBJECT:
                o << i->d.object;
                break;
            case JSON_ARRAY:
                o << i->d.array;
                break;
            case JSON_STRING:
                {
                    size_t *len = reinterpret_cast<size_t*>(i->d.str);
                    char *begin = reinterpret_cast<char*>(len+1);
                    char *end = begin + *len;

                    serialize_string(o, begin, end);
                }
                break;
            case JSON_INTEGER:
                o << i->d.i;
                break;
            case JSON_FLOAT:
                o << i->d.d;
                break;
        }
    }

    o << ']';

    return o;
}

std::ostream& operator<<(std::ostream& o, const JSONArray *array) {
    o << *array;
    return o;
}

/*
void JSONParser::parse(std::istream &is, JSONObject &object) {
{
    char c;
    std::vector<JSONValue> stack;
    JSONValue current;

    while(is.get(c).good()) {

        switch(parser->state) {
            case before_object:
                switch(c) {
                    case '{':
                        current.type = JSON_OBJECT;
                        current.d.object = new JSONObject();

                        if(current.d.object == NULL) {
                            throw std::runtime_error("not enough memory");
                        }

                        state = before_key;
                        break;
                    case '[':
                        current.type = JSON_ARRAY;
                        current.d.array = new JSONArray();

                        if(current.d.array == NULL) {
                            throw std::runtime_error("not enough memory");
                        }

                        state = before_element;
                        break;
                    case ' ': case '\t': case '\n': case '\r':
                        break;
                    default:
                        return NGX_ERROR;
                }
                break;
            case before_element:
                switch(c) {
                    case '"':
                        parser->state = inside_string;
                        break;
                    case 'n': case 'N':
                        parser->value.null = 1;
                        goto value;
                    case 't': case 'T': case 'f': case 'F':
                        parser->value.boolean = 1;
                        parser->value.d.numeric = (*p & 0xdf == 'T');
                        goto value;
                    case '-':
                        parser->negative = 1;
                    case '0': case '1': case '2': case '3':
                    case '4': case '5': case '6': case '7':
                    case '8': case '9':
                        parser->value.numeric = 1;
value:
                        if(parser->state == before_key) {
                            return NGX_ERROR;
                        }

                        parser->state = inside_value;
                        break;
                    case '}':
                        if(!(parser->hashes & 1)) {
                            return NGX_ERROR;
                        }

                        parser->object_end_handler(parser);

                        parser->arrays >>= 1;
                        parser->hashes >>= 1;
                        break;
                    case ']':
                        if(!(parser->arrays & 1)) {
                            return NGX_ERROR;
                        }

                        parser->array_end_handler(parser);

                        parser->arrays >>= 1;
                        parser->hashes >>= 1;
                        break;
                    case ' ': case '\t': case '\n': case '\r':
                        break;
                    default:
                        return NGX_ERROR;
                }
                break;
            case inside_string:
                switch(*p) {
                    case '"':
                        parser->state = after_value;
                        break;
                    case '\\':
                    default:
                        break;
                }
                break;
            case inside_boolean:
                switch(*p) {
                    case '"':
                        parser->state = after_value;
                        break;
                    case '\\':
                    default:
                        break;
                }
                break;
            case after_value:
                switch(*p) {
                    case ':':
                        if(!(parser->hashes & 1) || !(parser->keys & 1)) {
                            return NGX_ERROR;
                        }

                        parser->key_handler(parser, &parser->value);

                        parser->state = before_element;
                        break;
                    case ',':
                        if(!(parser->arrays & 1)) {
                            return NGX_ERROR;
                        }

                        parser->value_handler(parser, &parser->value);

                        parser->state = before_element;
                        break;
                    case ' ': case '\t': case '\n': case '\r':
                        break;
                }
                break;
        }

        p++;
    }
}
*/
