
#include <cstdio>
#include <stdexcept>
#include <fstream>
#include <cmath>

#include <openssl/sha.h>

#include <logger/logger.h>
#include <db/db_pool.h>

#include "url.h"
#include "xml.h"

#include "wp_handler.h"

#define REDIRECT_URL "/document.php?id="

namespace fp {

wp_handler::wp_handler(DBPool &_pool)
    : pool(_pool)
{
}

wp_handler::~wp_handler()
{
}

void wp_handler::init()
{
}

void wp_handler::shutdown() {
}

void wp_handler::return_error(fcgi_response &_response, const std::string &msg, int status) const {
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

void wp_handler::return_success(fcgi_request &_request, fcgi_response &_response, int document_id) const {
    std::string content_type(_request.get_param("showxml") != "yes" ? "text/xml" : "application/xml");

    XMLDoc doc("page");

    if(_request.get_param("showxml") != "yes") {
        doc.add_pi("modxslt-stylesheet", "type=\"text/xsl\" href=\"xsl/feedback.xsl\"");
    }

    XMLNode document("document");

    document.set_attr("id", document_id);

    doc.add(document.get());

    document.release();

    _response.fcgi_out << "Status: 200\r\n";
    _response.fcgi_out << "Content-Type: " << content_type << "\r\n";
    _response.fcgi_out << "\r\n";
    _response.fcgi_out << doc;
}

void wp_handler::handle_get(fcgi_request &_request, fcgi_response &_response)
{
    std::string content_type(_request.get_param("showxml") != "yes" ? "text/xml" : "application/xml");

    XMLDoc doc("page");

    if(_request.get_param("showxml") != "yes") {
        doc.add_pi("modxslt-stylesheet", "type=\"text/xsl\" href=\"xsl/feedback.xsl\"");
    }

    _response.fcgi_out << "Status: 200\r\n";
    _response.fcgi_out << "Content-Type: " << content_type << "\r\n";
    _response.fcgi_out << "\r\n";
    _response.fcgi_out << doc;
}

void wp_handler::handle_blogroll(fcgi_request &_request, fcgi_response &_response)
{
    std::string content_type(_request.get_param("showxml") != "yes" ? "text/xml" : "application/xml");

    try {
        DBConnHolder conn(pool);

        DBStmt stmt(conn.get(), "select id, post_title, post_excerpt, post_content from wp_posts where post_status='publish' "
            " and post_type='post' order by post_date_gmt desc limit 10");
        
        stmt.execute();

        XMLDoc doc("page");

        if(_request.get_param("showxml") != "yes") {
            doc.add_pi("modxslt-stylesheet", "type=\"text/xsl\" href=\"xsl/feedback.xsl\"");
        }

        XMLNode posts("posts");

        while(stmt.fetch()) {
            XMLNode post("post");

            post.set_attr("id", stmt.asInt(0));

            XMLNode title("title");
            title.add_text(stmt.asString(1));
            post.add(title.release());

            XMLNode excerpt("excerpt");
            excerpt.add_text(stmt.asString(2));
            post.add(excerpt.release());

            std::string content(stmt.asString(3));

            size_t more_pos = content.find("<!--more-->");

            XMLNode content_elm("content");
            if(more_pos != std::string::npos) {
                content_elm.set_attr("more", "yes");
                content_elm.add_text(content.substr(0, more_pos));
            }
            else {
                content_elm.add_text(content);
            }
            post.add(content_elm.release());

            posts.add(post.release());
        }

        doc.add(posts.release());

        _response.fcgi_out << "Status: 200\r\n";
        _response.fcgi_out << "Content-Type: " << content_type << "\r\n";
        _response.fcgi_out << "\r\n";
        _response.fcgi_out << doc;
    }
    catch(const db_exception &e) {
        LogError("wp_handler::handle DB error: " <<  e.what());
        return_error(_response, e.what());
    }
}

void wp_handler::handle_category(fcgi_request &_request, fcgi_response &_response, const std::string &name)
{
    LogInfo("wp_handler::handle_category name=" << name);

    try {
        DBConnHolder conn(pool);

        DBStmt stmt(conn.get(), "select ID from wp_posts where post_status='publish' and post_name=?");

        stmt.bindString(1, name);
        
        stmt.execute();

        if(!stmt.fetch()) {
        }
    }
    catch(const db_exception &e) {
        LogError("wp_handler::handle DB error: " <<  e.what());
        return return_error(_response, e.what());
    }
}

void wp_handler::handle_author(fcgi_request &_request, fcgi_response &_response, const std::string &name)
{
    LogInfo("wp_handler::handle_author name=" << name);

    try {
        DBConnHolder conn(pool);

        DBStmt stmt(conn.get(), "select ID from wp_posts where post_status='publish' and post_name=?");

        stmt.bindString(1, name);
        
        stmt.execute();

        if(!stmt.fetch()) {
        }
    }
    catch(const db_exception &e) {
        LogError("wp_handler::handle DB error: " <<  e.what());
        return return_error(_response, e.what());
    }
}

bool wp_handler::handle_post(fcgi_request &_request, fcgi_response &_response, const std::string &name)
{
    std::string content_type(_request.get_param("showxml") != "yes" ? "text/xml" : "application/xml");

    LogInfo("wp_handler::handle_post name=" << name);

    try {
        DBConnHolder conn(pool);

        DBStmt stmt(conn.get(), "select id, post_content from wp_posts where post_status='publish' and post_name=?");

        stmt.bindString(0, name);
        
        stmt.execute();

        if(!stmt.fetch()) {
            return false;
        }

        XMLDoc doc("page");

        if(_request.get_param("showxml") != "yes") {
            doc.add_pi("modxslt-stylesheet", "type=\"text/xsl\" href=\"xsl/feedback.xsl\"");
        }

        XMLNode post("post");

        post.set_attr("id", stmt.asInt(0));

        post.add_text(stmt.asString(1));

        doc.add(post.release());

        _response.fcgi_out << "Status: 200\r\n";
        _response.fcgi_out << "Content-Type: " << content_type << "\r\n";
        _response.fcgi_out << "\r\n";
        _response.fcgi_out << doc;
    }
    catch(const db_exception &e) {
        LogError("wp_handler::handle DB error: " <<  e.what());
        return_error(_response, e.what());
        return true;
    }

    return true;
}

void wp_handler::handle_404(fcgi_request &_request, fcgi_response &_response)
{
    LogInfo("wp_handler::handle_404");
    return_error(_response, "page does not exist", 404);
}

void wp_handler::handle(fcgi_request &_request, fcgi_response &_response)
{
    std::string script_name = _request.get_fcgi_param("SCRIPT_NAME");

    if(script_name == "/" || script_name == "") {
        handle_blogroll(_request, _response);
    }
    else if(script_name.find("/category/") == 0) {
        handle_category(_request, _response, script_name.substr(sizeof("/category/") - 1));
    }
    else if(script_name.find("/author/") == 0) {
        handle_author(_request, _response, script_name.substr(sizeof("/author/") - 1));
    }
    else if(!handle_post(_request, _response, script_name.substr(1))) {
        return handle_404(_request, _response);
    }
}

#if 0
std::string wp_handler::get_email_hash(const std::string &email)
{
    static const char hextab[] = "0123456789abcdef";
    SHA_CTX sha_ctx;
    unsigned char sha_hash[20];
    std::ostringstream result;

    SHA_Init(&sha_ctx);
    SHA_Update(&sha_ctx, email.c_str(), email.size());
    SHA_Final(sha_hash, &sha_ctx);

    for(size_t i = 0 ; i != 20 ; i++) {
        result << hextab[(sha_hash[i] >> 4) & 0xf];
        result << hextab[sha_hash[i] & 0xf];
    }

    return result.str();
}
#endif

};

