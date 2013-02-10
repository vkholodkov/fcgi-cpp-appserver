
#ifndef _GEARMAN_CLIENT_H_
#define _GEARMAN_CLIENT_H_

#include <libgearman/gearman.h>

#include "gearman.h"

class GearmanClient {
public:
    GearmanClient(const std::string &_host, int port = GEARMAN_DEFAULT_TCP_PORT, int timeout = -1)
    {
        gearman_return_t ret;

        if(gearman_client_create(&client) == NULL)
        {
            throw gearman_exception("Memory allocation failure on client creation"); 
        }

        ret = gearman_client_add_server(&client, _host.c_str(), port);

        if(ret != GEARMAN_SUCCESS)
        {
            throw gearman_exception(&client);
        }

        if(timeout >= 0)
            gearman_client_set_timeout(&client, timeout);
    }

    ~GearmanClient()
    {
        gearman_client_free(&client);
    }

    std::string run_bg(const std::string &function, const std::string &workload)
    {
        gearman_return_t ret;
        char job_handle[GEARMAN_JOB_HANDLE_SIZE];

        ret = gearman_client_do_background(&client, function.c_str(), NULL, workload.c_str(), workload.size(), job_handle);

        if(ret != GEARMAN_SUCCESS)
        {
            throw gearman_exception(&client);
        }

        return std::string(job_handle);
    }

    std::string run_bg_high(const std::string &function, const std::string &workload)
    {
        gearman_return_t ret;
        char job_handle[GEARMAN_JOB_HANDLE_SIZE];

        ret = gearman_client_do_high_background(&client, function.c_str(), NULL, workload.c_str(), workload.size(), job_handle);

        if(ret != GEARMAN_SUCCESS)
        {
            throw gearman_exception(&client);
        }

        return std::string(job_handle);
    }

private:
    gearman_client_st client;
};

#endif
