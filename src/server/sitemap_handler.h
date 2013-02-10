
#ifndef _SITEMAP_HANDLER_H_
#define _SITEMAP_HANDLER_H_

#include <memory>

#include <string>
#include <set>

#include <db/db_pool.h>

#include "fcgi_handler.h"
#include "xml.h"

namespace fp {

class sitemap_handler : public fp::fcgi_handler {
    typedef std::set<std::string> CategoryContainer;
public:
    sitemap_handler(DBPool&);
    virtual ~sitemap_handler();

    virtual void init();
    virtual void shutdown();

    virtual void handle(fcgi_request &_request, fcgi_response &_response);

private:
    void return_error(fcgi_response&, const std::string&, int status = 200) const;

    static std::string get_base_url(DBConn&);
    static void add_url(XMLDoc&, const std::string&, const std::string&, const std::string&, double);

    DBPool &pool;
};

};

#endif
