
#ifndef _GEARMAN_WORKER_H_
#define _GEARMAN_WORKER_H_

#include <libgearman/gearman.h>

#include "gearman.h"

class GearmanWorker {
public:
    GearmanWorker(const std::string &_host, int port = 0, int timeout = -1)
    {
        gearman_return_t ret;

        if(gearman_worker_create(&worker) == NULL)
        {
            throw gearman_exception("Memory allocation failure on worker creation"); 
        }

        ret = gearman_worker_add_server(&worker, _host.c_str(), port);
        if(ret != GEARMAN_SUCCESS)
        {
            throw gearman_exception(&worker);
        }
    }

    ~GearmanWorker()
    {
        gearman_worker_free(&worker);
    }

    void add_function(const std::string &name, gearman_worker_fn *function, void *context = 0)
    {
        gearman_return_t ret;

        ret = gearman_worker_add_function(&worker, name.c_str(), 0, function, context);
        if(ret != GEARMAN_SUCCESS)
        {
            throw gearman_exception(&worker);
        }
    }

    void work()
    {
        gearman_return_t ret;

        ret = gearman_worker_work(&worker);
        if(ret != GEARMAN_SUCCESS)
        {
            throw gearman_exception("Memory allocation failure on worker creation"); 
        }
    }

private:
    gearman_worker_st worker;
};

#endif
