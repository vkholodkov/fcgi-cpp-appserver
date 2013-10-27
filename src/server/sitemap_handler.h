
#ifndef _SITEMAP_HANDLER_H_
#define _SITEMAP_HANDLER_H_

#include <memory>

#include <string>
#include <set>
#include <map>

#include <db/db_pool.h>

#include "fcgi_handler.h"
#include "xml.h"

namespace fp {

class sitemap_handler : public fp::fcgi_handler {
    typedef std::set<std::string> CategoryContainer;
    typedef std::map<std::string, DBPool*> PoolContainer;
public:
    sitemap_handler();
    virtual ~sitemap_handler();

    virtual void init();
    virtual void shutdown();

    virtual void handle(fcgi_request &_request, fcgi_response &_response);

    void add_site(const std::string &hostname, DBPool &pool)
    {
        sites.insert(std::make_pair(hostname, &pool));
    }

private:
    void return_error(fcgi_response&, const std::string&, int status = 200) const;

    static std::string get_base_url(DBConn&);
    size_t get_num_posts_per_page(DBConn&);
    static void add_url(XMLDoc&, const std::string&, const std::string&, const std::string&, double);

    PoolContainer sites;
};

};

#endif
