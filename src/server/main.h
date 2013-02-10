
#ifndef _MAIN_H_
#define _MAIN_H_

#include <signal.h>

#include <map>

typedef int (*worker_function_fp)();

class MainProcess {
public:
    MainProcess()
        : m_workers()
        , m_functions()
        , m_last_respawn(::time(NULL) - 5)
    {
    }

    typedef std::map<pid_t, worker_function_fp> worker_pool;
    typedef std::map<worker_function_fp, std::pair<size_t, size_t> > worker_functions;

    void run();
    static void exit() { m_exiting = 1; };

    void add_function(worker_function_fp fp, size_t min_workers)
    {
        m_functions.insert(std::make_pair(fp, std::make_pair(0, min_workers)));
    }

    bool functions_defined() const{ return !m_functions.empty(); }

private:
    void spawn();

private:
    static sig_atomic_t m_exiting;
    worker_pool m_workers;
    worker_functions m_functions;
    time_t m_last_respawn;
};

#endif
