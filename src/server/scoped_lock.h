
#ifndef _SCOPED_LOCK_
#define _SCOPED_LOCK_

#include <pthread.h>

class ScopedRLock {
public:
    ScopedRLock(pthread_rwlock_t &_rwlock)
        : m_rwlock(_rwlock)
    {
        pthread_rwlock_rdlock(&m_rwlock);
    }

    ~ScopedRLock()
    {
        pthread_rwlock_unlock(&m_rwlock);
    }

private:
    pthread_rwlock_t &m_rwlock;
};

class ScopedWLock {
public:
    ScopedWLock(pthread_rwlock_t &_rwlock)
        : m_rwlock(_rwlock)
    {
        pthread_rwlock_wrlock(&m_rwlock);
    }

    ~ScopedWLock()
    {
        pthread_rwlock_unlock(&m_rwlock);
    }

private:
    pthread_rwlock_t &m_rwlock;
};

#endif //_SCOPED_LOCK_
