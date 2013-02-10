
#include <cstdio>
#include <stdexcept>
#include <fstream>
#include <cmath>

#include <openssl/sha.h>

#include <logger/logger.h>
#include <db/db_pool.h>

#include "xml.h"

#include "sitemap_handler.h"

namespace fp {

sitemap_handler::sitemap_handler(DBPool &_pool)
    : pool(_pool)
{
}

sitemap_handler::~sitemap_handler()
{
}

void sitemap_handler::init()
{
}

void sitemap_handler::shutdown() {
}

void sitemap_handler::return_error(fcgi_response &_response, const std::string &msg, int status) const {
    _response.fcgi_out << "Status: " << status << "\r\n";
    _response.fcgi_out << "Content-Type: text/html\r\n";
    _response.fcgi_out << "\r\n";

    _response.fcgi_out << "<html>";
    _response.fcgi_out << "<head>";
    _response.fcgi_out << "<title>Error</title>";
    _response.fcgi_out << "</head>";
    _response.fcgi_out << "<body>";
    _response.fcgi_out << "<h2>An error has occured</h2>";
    _response.fcgi_out << "<p>" << msg << "</p>";
    _response.fcgi_out << "</body>";
    _response.fcgi_out << "</html>";
}

std::string sitemap_handler::get_base_url(DBConn &conn)
{
    DBStmt stmt(conn, "select option_value from wp_options where option_name='home'");
    
    stmt.execute();

    return stmt.fetch() ? stmt.asString(0) : "";
}

void sitemap_handler::add_url(XMLDoc &doc, const std::string &loc, const std::string &lastmod, const std::string &changefreq, double priority)
{
    XMLNode url_elm("url");

    XMLNode loc_elm("loc");
    loc_elm.add_text(loc);
    url_elm.add(loc_elm.release());

    XMLNode lastmod_elm("lastmod");
    lastmod_elm.add_text(lastmod);
    url_elm.add(lastmod_elm.release());

    XMLNode changefreq_elm("changefreq");
    changefreq_elm.add_text(changefreq);
    url_elm.add(changefreq_elm.release());

    std::ostringstream priority_str;
    priority_str << priority;

    XMLNode priority_elm("priority");
    priority_elm.add_text(priority_str.str());
    url_elm.add(priority_elm.release());

    doc.add(url_elm.release());
}

void sitemap_handler::handle(fcgi_request &_request, fcgi_response &_response)
{
    try {
        DBConnHolder conn(pool);

        std::string base_url(get_base_url(conn.get()));

        XMLDoc doc("urlset", "http://www.sitemaps.org/schemas/sitemap/0.9", "1.0");

//        doc.add_pi("xml-stylesheet", "type=\"text/xsl\" href=\"wp-content/plugins/google-sitemap-plugin/sitemap.xsl\"");

        {
            /*
             * Add page URLs
             */
            DBStmt stmt(conn.get(), "select post_name, post_modified_gmt from wp_posts where post_status='publish' "
                " and post_type='page' order by post_date_gmt");
            
            stmt.execute();

            while(stmt.fetch()) {
                std::ostringstream url;
                std::string modified_dt(stmt.asString(1));

                url << base_url << '/' << stmt.asString(0) << '/';

                modified_dt[10] = 'T';

                add_url(doc, url.str(), modified_dt + "+00:00", "weekly", 1.0);
            }
        }

        {
            /*
             * Add post URLs
             */
            DBStmt stmt(conn.get(), "select post_name, post_modified_gmt, post_date_gmt from wp_posts where post_status='publish' "
                " and post_type='post' order by post_date_gmt");
            
            stmt.execute();

            while(stmt.fetch()) {
                std::ostringstream url;
                std::string dt(stmt.asString(2));
                std::string modified_dt(stmt.asString(1));

                url << base_url << '/' << dt.substr(0, 4) << '/' << dt.substr(5, 2) << '/' << stmt.asString(0) << '/';

                modified_dt[10] = 'T';

                add_url(doc, url.str(), modified_dt + "+00:00", "monthly", 1.0);
            }
        }

        {
            std::string taxonomy("category");
            /*
             * Add category URLs
             */
            DBStmt stmt(conn.get(), "select slug, max(p.post_modified_gmt) from wp_terms ts, wp_term_taxonomy tt, wp_term_relationships tr, wp_posts p"
                 " where tt.taxonomy='" + taxonomy + "' and tt.term_id=ts.term_id and tt.parent=0 and tt.count!=0 and "
                 " tt.term_taxonomy_id = tr.term_taxonomy_id and tr.object_id=p.ID and p.post_status='publish' and p.post_type='post' group by slug");
            
            stmt.execute();

            while(stmt.fetch()) {
                std::ostringstream url;
                std::string modified_dt(stmt.asString(1));

                url << base_url << '/' << taxonomy << '/' << stmt.asString(0) << '/';

                modified_dt[10] = 'T';

                add_url(doc, url.str(), modified_dt+"+00:00", "daily", 1.0);
            }
        }

        {
            /*
             * Add author URLs
             */
            DBStmt stmt(conn.get(), "select user_login, max(p.post_modified_gmt) from wp_posts p, wp_users u where p.post_author=u.ID "
                " and p.post_status='publish' and p.post_type='post' group by user_login");
            
            stmt.execute();

            while(stmt.fetch()) {
                std::ostringstream url;
                std::string modified_dt(stmt.asString(1));

                url << base_url << "/author/" << stmt.asString(0) << '/';

                modified_dt[10] = 'T';

                add_url(doc, url.str(), modified_dt+"+00:00", "daily", 1.0);
            }
        }

        _response.fcgi_out << "Status: 200\r\n";
        _response.fcgi_out << "Content-Type: application/xml\r\n";
        _response.fcgi_out << "\r\n";
        _response.fcgi_out << doc;
    }
    catch(const db_exception &e) {
        LogError("wp_handler::handle DB error: " <<  e.what());
        return_error(_response, e.what());
    }
}

};

