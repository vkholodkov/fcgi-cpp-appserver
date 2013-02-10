
#ifndef _URL_H_
#define _URL_H_

#include <string>

class URL {
public:
    URL(const std::string &_source)
        : source(_source)
    {
        std::string::const_iterator s, p, q;

        p = source.begin();
        q = source.end();

        if(source.size() >= sizeof("http://")-1) {
            if(p[0] == 'h' && p[1] == 't' && p[2] == 't' && p[3] == 'p') {
                if(p[4] == 's' && p[5] == ':' && p[6] == '/' && p[7] == '/') {
                    schema.assign(_source, 0, 8);
                    p += 8;
                }
                else if(p[4] == ':' && p[5] == '/' && p[6] == '/') {
                    schema.assign(_source, 0, 7);
                    p += 7;
                }
            }
        }

        s = p;

        while(p != q) {
            if(*p == ':' || *p == '/' || *p == '?') {
                break;
            }
            p++;
        }

        host.assign(s, p);

        s = p;

        if(p != q && *p == ':') {
            p++;
            s = p;
            while(p != q) {
                if(!(*p >= '0' && *p <= '9')) {
                    break;
                }
                p++;
            }
            port.assign(s, p);

            if(port.empty()) {
                throw std::runtime_error("bad url");
            }
        }

        s = p;

        if(p != q && *p == '/') {
            while(p != q) {
                if(*p == '?') {
                    break;
                }
                p++;
            }
            path.assign(s, p);
        }

        s = p;

        if(p != q && *p == '?') {
            while(p != q) {
                p++;
            }
            query.assign(s, p);
        }
    }

    std::string source;
    std::string schema;
    std::string host;
    std::string port;
    std::string path;
    std::string query;
};

#endif
