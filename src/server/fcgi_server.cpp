
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sstream>
#include <stdexcept>

#include <sys/types.h>
#include <sys/socket.h>

#include <fcgiapp.h>
#include <fcgio.h>

#include <logger/logger.h>

#include "config.h"

#include "fcgi_server.h"
#include "fcgi_handler.h"

namespace fp {

sig_atomic_t fcgi_server::m_exiting = false;

fcgi_server::fcgi_server(const std::string &_endpoint)
    : endpoint(_endpoint)
{
    int rc;

    rc = pthread_mutex_init(&accept_mutex, NULL);

    if(rc != 0)
        throw fcgi_server_exception("cannot initialize accept mutex");
}

fcgi_server::~fcgi_server()
{
    for(handler_list_t::iterator i = handlers.begin() ; i != handlers.end() ; i++ )
        delete *i;

    pthread_mutex_destroy(&accept_mutex);
}

void fcgi_server::init() {
    FCGX_Init();

    listening_socket = FCGX_OpenSocket(endpoint.c_str(), 5);

    if(listening_socket < 0)
        throw fcgi_server_exception("cannot create listening socket");
    

    // For now just call all handlers 
    for(handler_list_t::iterator i = handlers.begin() ; i != handlers.end() ; i++ )
        (*i)->init();
}

void fcgi_server::shutdown() {
    for(handler_list_t::iterator i = handlers.begin() ; i != handlers.end() ; i++ )
        (*i)->shutdown();

    close(listening_socket);
}

void fcgi_server::run() {
    void *return_value; 

    signal(SIGPIPE, SIG_IGN);
    
    for(int i = 0 ; i < NUM_WORKERS ; i++ ) {
        pthread_create(worker_threads + i, NULL, fcgi_server::worker_thread_starter, (void*)this);
    }

    for(int i = 0 ; i < NUM_WORKERS ; i++ ) {
        pthread_join(*(worker_threads + i), &return_value);
    }
}

void fcgi_server::worker_thread() {
	using namespace std;

    signal(SIGPIPE, SIG_IGN);

    FCGX_Request request;
    
    LogInfo("worker thread started");

    FCGX_InitRequest(&request, listening_socket, 0);

    while(!m_exiting) {
        int rc;

        pthread_mutex_lock(&accept_mutex);
        rc = FCGX_Accept_r(&request);
        pthread_mutex_unlock(&accept_mutex);

        if (rc < 0) {
            LogInfo("accept failed: " << rc);
            break;
        }

        // Construct request and response and invoke handler
        fcgi_request fcgi_req(request);
        fcgi_response fcgi_resp(request);

        try{
            if(FCGX_GetParam("SCRIPT_NAME", request.envp) == NULL)
                throw std::logic_error("HTTP server is not configured to pass SCRIPT_NAME variable");

            invoke_handler(fcgi_req.get_script_name(), fcgi_req, fcgi_resp);

        }catch(const fp::fcgi_exception &e) {
            fcgi_resp.fcgi_out << "Status: " << e.status() << "\r\nContent-Type: text/plain; encoding=utf-8\r\n\r\nError: " << e.what();
        }catch(const std::exception &e) {
            fcgi_resp.fcgi_out << "Status: 500\r\nContent-Type: text/plain; encoding=utf-8\r\nCache-Control: no-cache\r\n\r\nError: " << e.what();
        }catch(...) {
            fcgi_resp.fcgi_out << "Status: 500\r\nContent-Type: text/plain; encoding=utf-8\r\nCache-Control: no-cache\r\n\r\nUnknown internal error";
        }

        // Ensure all data is sent
        fcgi_resp.fcgi_out.flush();

        FCGX_Finish_r(&request);
    }

    LogInfo("worker thread terminated");
}

void *fcgi_server::worker_thread_starter(void *arg) {
    fcgi_server *s;

    s = static_cast<fcgi_server*>(arg);

    s->worker_thread();

    return NULL;
}

void fcgi_server::add_handler(fcgi_handler *_handler)
{
    if(_handler != 0)
        handlers.push_back(_handler);
}

void fcgi_server::remove_handler(fcgi_handler *_handler)
{
    handlers.remove(_handler);
}

void fcgi_server::add_handler_mapping(const std::string &_uri, fcgi_handler *_handler) {
    if(_uri.find('=') == 0) {
        exact_locations_map.insert(std::make_pair(_uri.substr(sizeof("=")), _handler));
    }
    else {
        locations_map.insert(std::make_pair(_uri, _handler));
    }
}

void fcgi_server::remove_handler_mapping(const std::string &_uri, fcgi_handler *_handler) {
}

void fcgi_server::invoke_handler(const std::string &_uri, fcgi_request &_request, fcgi_response &_response) const {
    std::string matched_prefix;
    fcgi_handler* handler_to_fire;

    // Find and call handlers by exact match
    std::pair<location_map_t::const_iterator, location_map_t::const_iterator>
        handlers_to_invoke = exact_locations_map.equal_range(_uri);

    handler_to_fire = 0;
    
    for(location_map_t::const_iterator i = handlers_to_invoke.first ; i != handlers_to_invoke.second ; i++ )
    {
        if(i->second != 0) {
            if(matched_prefix.size() < i->first.size()) { 
                handler_to_fire = i->second;
            }
        }
    }

    if(handler_to_fire != 0) {
        handler_to_fire->handle(_request, _response);
        return;
    }

    location_map_t::const_iterator start = locations_map.begin();
    location_map_t::const_iterator end = locations_map.end();

    matched_prefix.clear();
    handler_to_fire = 0;
    
    for(location_map_t::const_iterator i = start ; i != end ; i++ )
    {
        if(_uri.find(i->first) == 0 && i->second != 0) {
            if(matched_prefix.size() < i->first.size()) { 
                handler_to_fire = i->second;
            }
        }
    }

    if(handler_to_fire != 0) {
        handler_to_fire->handle(_request, _response);
        return;
    }

    throw fcgi_exception(("No handlers configured for location: \"" + _uri + "\"").c_str(), HTTP_NOT_FOUND);
}

};

