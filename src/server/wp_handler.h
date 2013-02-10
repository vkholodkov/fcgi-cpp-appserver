
#ifndef _WP_HANDLER_H_
#define _WP_HANDLER_H_

#include <memory>

#include <string>
#include <set>

#include <db/db_pool.h>

#include "fcgi_handler.h"

namespace fp {

class wp_handler : public fp::fcgi_handler {
    typedef std::set<std::string> CategoryContainer;
public:
    wp_handler(DBPool&);
    virtual ~wp_handler();

    virtual void init();
    virtual void shutdown();

    virtual void handle(fcgi_request &_request, fcgi_response &_response);

private:
    void handle_get(fcgi_request&, fcgi_response&);
    void handle_blogroll(fcgi_request&, fcgi_response&);
    void handle_category(fcgi_request&, fcgi_response&, const std::string&);
    void handle_author(fcgi_request&, fcgi_response&, const std::string&);
    bool handle_post(fcgi_request&, fcgi_response&, const std::string&);
    void handle_404(fcgi_request&, fcgi_response&);

    void return_error(fcgi_response&, const std::string&, int status = 200) const;
    void return_success(fcgi_request&, fcgi_response&, int) const;

    DBPool &pool;
};

};

#endif
