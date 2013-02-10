
#ifndef _FCGI_SERVER_H_
#define _FCGI_SERVER_H_

#include <exception>
#include <set>
#include <map>
#include <list>
#include <signal.h>

#include <fcgio.h>

#include "fcgi_handler.h"

#define NUM_WORKERS 5

namespace fp {

class fcgi_server_exception : public std::exception {
public:
    fcgi_server_exception(const std::string &_error_message)
        : error_message(_error_message)
    {
    }

    virtual ~fcgi_server_exception() throw()
    {
    }

public:
    std::string get_error_message() const { return error_message; }

private:
    std::string error_message;
};

class fcgi_server {
private:
    typedef std::multimap<std::string,fcgi_handler*> location_map_t;
    typedef std::list<fcgi_handler*> handler_list_t;

public:
    fcgi_server(const std::string &_endpoint = "127.0.0.1:9002");
    ~fcgi_server();
    
    /*
     * Add handler to server's handlers list
     *
     * @param _handler pointer to handler object to be added. 
     *                 Server becomes responsible for destroying handler being added
     */
    void add_handler(fcgi_handler *_handler);

    /*
     * Remove handler from server's handlers list
     *
     * @param _handler pointer to handler object to be removed. 
     *                 Caller becomes responsible for destroying handler being removed
     */
    void remove_handler(fcgi_handler *_handler);

    /*
     * Add handler mapping to server's handler mapping list
     *
     * @param _uri URI template to map
     * @param _handler pointer to handler object to be mapped. 
     */
    void add_handler_mapping(const std::string &_uri, fcgi_handler *_handler);

    void remove_handler_mapping(const std::string &_uri, fcgi_handler *_handler);

    void invoke_handler(const std::string &_uri, fcgi_request &, fcgi_response &) const;
   
    /*
     * Initialize server
     */
    void init();

    /*
     * Run server. Returns when all worker threads terminated
     */
    void run();

    /*
     * Shutdown server.
     */
    void shutdown();

    static void exit() { m_exiting = true; }

private:
    void worker_thread();
    static void *worker_thread_starter(void *arg);

private:
    const std::string endpoint;
    pthread_t worker_threads[NUM_WORKERS];

    pthread_mutex_t accept_mutex;

    location_map_t locations_map;
    location_map_t exact_locations_map;
    handler_list_t handlers;

    int listening_socket;

    unsigned min_threads, max_threads;
    static sig_atomic_t m_exiting;
};

};

#endif //_FCGI_SERVER_H_

