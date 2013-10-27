
#include <signal.h>
#include <memory>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include <logger/logger.h>

#include "wp_handler.h"
#include "sitemap_handler.h"

#include "fcgi_server.h"

using namespace fp;

void drop_permissions()
{
    pid_t pid;
    struct passwd *nobody;
    struct group *nogroup;

#if 0
    nobody = getpwnam("awww-data");

    if(nobody == NULL) {
        nobody = getpwnam("nobody");
    }

    if(nobody == NULL) {
        std::cout << "no nobody or www-data user in /etc/passwd" << std::endl;
        return -1;
    }

    nogroup = getgrnam("awww-data");

    if(nogroup == NULL) {
        nogroup = getgrnam("nogroup");
    }

    if(nobody == NULL) {
        std::cout << "no nogroup or www-data user in /etc/group" << std::endl;
        return -1;
    }

    setsid();

    setuid(nobody->pw_uid);
    setgid(nogroup->gr_gid);
#endif

    close(0); close(1); close(2);

    signal(SIGPIPE,SIG_IGN);
}

static void sigquit_handler(int sig)
{
    ::exit(0);
}

int worker_process()
{
    struct sigaction new_action;

    new_action.sa_handler = sigquit_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGQUIT, &new_action, NULL);

    DBPool pool_vkholodkov_blog(DBCred("localhost", "vkholodkov_blog", "vkholodkov_blog", "vkholodkov_blog"), 5, 40);
    DBPool pool_wp_com(DBCred("localhost", "wp_com", "wp_com", "wp_com"), 5, 40);

    DBPool::start_maintenance_thread(pool_vkholodkov_blog);
    DBPool::start_maintenance_thread(pool_wp_com);

    try{
        fcgi_server s(":9002");

        drop_permissions();

        std::auto_ptr<fcgi_handler> wp_handler_ptr(new wp_handler(pool_vkholodkov_blog));
        std::auto_ptr<sitemap_handler> sitemap_handler_ptr(new sitemap_handler());

        sitemap_handler_ptr->add_site("www.nginxguts.com", pool_vkholodkov_blog);
        sitemap_handler_ptr->add_site("www.weddingpastry.com", pool_wp_com);
        sitemap_handler_ptr->add_site("dev.weddingpastry.com", pool_wp_com);

        s.add_handler_mapping("/", wp_handler_ptr.get());
        s.add_handler_mapping("/sitemap.xml", sitemap_handler_ptr.get());

        s.add_handler(sitemap_handler_ptr.release());
        s.add_handler(wp_handler_ptr.release());

        LogInfo("worker process " << getpid() << " started");

        s.init();

        s.run();

        s.shutdown();
    }catch(const fcgi_server_exception &e) {
        LogInfo("worker caught server_exception: " << e.get_error_message());
        return 1;
    }catch(...) {
        LogInfo("worker caught unknown exception");
        return 1;
    }

    DBPool::join_maintenance_thread();

    LogInfo("worker process exited");

    return 0;
}
