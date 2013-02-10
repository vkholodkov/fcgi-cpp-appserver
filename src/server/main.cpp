
#include <signal.h>
#include <memory>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <getopt.h>

#include <logger/logger.h>

#include "submit_handler.h"

#include "fcgi_server.h"

#include "main.h"

using namespace fp;

static bool run_main_process = true;

void init_classifier();

int worker_process();
int linker_worker();

sig_atomic_t MainProcess::m_exiting = 0;

static void sigquit_handler(int sig)
{
    MainProcess::exit();
}

void MainProcess::run()
{
    bool exited = false;
    bool signalled = false;
    struct sigaction new_action;

    if(run_main_process) {
        LogInfo("main process " << getpid() << " started");

        new_action.sa_handler = sigquit_handler;
        sigemptyset(&new_action.sa_mask);
        new_action.sa_flags = 0;
        sigaction(SIGQUIT, &new_action, NULL);
        sigaction(SIGINT, &new_action, NULL);
        sigaction(SIGTERM, &new_action, NULL);

        spawn();

        do {
            pid_t pid;
            int status;

            pid = waitpid(-1, &status, 0);

            if(pid == -1) {
                if(errno != EINTR && errno != ECHILD) {
                    LogError("waitpid failed: " << errno);
                }
            }
            else {
                worker_pool::iterator i = m_workers.find(pid);

                if(i != m_workers.end()) {
                    if(WIFEXITED(status)) {
                        LogInfo("worker process " << pid << " finished with status " << WEXITSTATUS(status));
                    }
                    else if(WIFSIGNALED(status) ) {
                        if(!WCOREDUMP(status)) {
                            LogInfo("worker process " << pid << " terminated by signal " << WTERMSIG(status));
                        }
                        else {
                            LogInfo("worker process " << pid << " terminated by signal " << WTERMSIG(status) << " (core dumped)");
                        }
                    }

                    m_workers.erase(i);
                    m_functions[i->second].first--;
                }
                else {
                    LogInfo("got notification for process " << pid << " but it is not my child");
                }
            }

            if(m_exiting) {
                if(!signalled) {
                    for(worker_pool::const_iterator i = m_workers.begin() ; i != m_workers.end() ; i++) {
                        LogInfo("signalling " << i->first);
                        ::kill(i->first, SIGQUIT);
                    }
                    signalled = true;
                }

                if(m_workers.empty()) {
                    exited = true;
                }
            }
            else {
                spawn();
            }

        } while(!exited);

        LogInfo("main process finished");
    }
    else {
        ::exit(worker_process());
    }
}

void MainProcess::spawn()
{
    pid_t pid;
#if 0
    if(time(NULL) - m_last_respawn < 5) {
        return;
    }
#endif
    m_last_respawn = time(NULL);

    for(worker_functions::iterator i = m_functions.begin() ; i != m_functions.end() ; i++) {
        while(i->second.first < i->second.second) {
            switch(pid = fork()) {
                case 0:
                    ::exit(i->first());
                    break;
                case -1:
                    LogError("fork failed: " << errno);
                    return;
                default:
                    m_workers.insert(std::make_pair(pid, i->first));
                    i->second.first++;
                    break;
            }
        }
    }
}

static struct option long_options[] = {
    {0, 0, 0, 0}
};

int main(int argc,char *argv[])
{
    int                 c;
    int                 this_option_optind;
    int                 option_index;
    pid_t               pid;
    bool                daemon = true;
    MainProcess         main_process;
    const char          *conf_file_path = "pm_backend.conf";

    while(1) {
        this_option_optind = optind ? optind : 1;
        option_index = 0;

        c = getopt_long(argc, argv, "c:nmh?", long_options, &option_index);

        if(c == -1) {
             break;
        }

        switch(c) {
            case 0:
                std::cout << "unknown option " << long_options[option_index].name;
                if(optarg)
                    std::cout << " with arg " << optarg;
                std::cout << std::endl;
                break;

            case 'n':
                daemon = false;
                break;

            case 'm':
                run_main_process = false;
                break;

            case 'c':
                conf_file_path = optarg;
                break;

            case 'h': case '?':
                break;
        }
    }
#if 0
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(conf_file_path, pt);
    std::cout << pt.get<std::string>("Section1.Value1") << std::endl;
    std::cout << pt.get<std::string>("Section1.Value2") << std::endl;
#endif
    main_process.add_function(worker_process, 1);   

    if(!main_process.functions_defined()) {
        std::cerr << "No workers defined" << std::endl;
        return -1;
    }

    if(daemon) {
        pid = fork();

        switch(pid) {
            case 0:
                /*
                 * Start session, close files and drop privileges
                 */
                setsid();

                close(0); close(1); close(2);

                main_process.run();

                exit(0);
                break;
            case -1:
                LogError("fork() failed: " << strerror(errno));
                break;
            default:
                break;
        }
    }
    else {
        main_process.run();
    }

    return 0;
}

