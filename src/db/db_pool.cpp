
#include <db/db_pool.h>

std::list<DBPool*> DBPool::pools;
pthread_t DBPool::maintenance_thread;
pthread_mutex_t DBPool::startup_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t DBPool::maintenance_lock = PTHREAD_MUTEX_INITIALIZER;
volatile bool DBPool::maintenance_thread_running = false;
volatile bool DBPool::maintenance_thread_exiting = false;

void *DBPool::maintenance_thread_func(void *arg) {
    bool exiting;

    do {
        time_t now;

        time(&now);

        pthread_mutex_lock(&maintenance_lock);

        for(std::list<DBPool*>::iterator p = pools.begin() ; p != pools.end() ; p++) {

            std::auto_ptr<DBConn> to_remove;

            /*
             * Look for a connection that is idle for more than 5 minutes
             */
            pthread_mutex_lock(&(*p)->m_lock);
            for(std::list<DBConn*>::iterator i = (*p)->m_available_connections.begin() ; i != (*p)->m_available_connections.end() ; i++) {
                if((now - (*i)->m_last_used) >= 60) {
                    to_remove.reset(*i);
                    (*p)->m_available_connections.erase(i);
                    break;
                }
            }
            pthread_mutex_unlock(&(*p)->m_lock);
        }

        exiting = maintenance_thread_exiting;

        pthread_mutex_unlock(&maintenance_lock);

        sleep(1);
    } while(!exiting);

    return NULL;
}

void DBPool::start_maintenance_thread(DBPool &pool)
{
    pthread_mutex_lock(&startup_lock);

    pools.push_back(&pool);

    if(!maintenance_thread_running) {
        pthread_create(&maintenance_thread, NULL, DBPool::maintenance_thread_func, NULL);
        maintenance_thread_running = true;
    }

    pthread_mutex_unlock(&startup_lock);
}

void DBPool::join_maintenance_thread()
{
    void *exit_code;

    pthread_mutex_lock(&maintenance_lock);
    maintenance_thread_exiting = true;
    pthread_mutex_unlock(&maintenance_lock);

    pthread_join(DBPool::maintenance_thread, &exit_code);
}

