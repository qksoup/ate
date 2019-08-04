#include <ate/ate.hpp>

#include <time.h>

namespace {
    const unsigned int pauseCountMax = 32;
    const unsigned int yieldCountMax = 8;
    const unsigned int sleepCountMax = 512;

    void* threadFunc(void* param)
    {
        ate::Thread* thread = (ate::Thread*)param;
        thread->run();   
        return NULL;
    }
}

namespace ate {
    Thread::Thread(bool joinable) : 
        m_joinable(joinable)
    {
        ATE_ASSERT(pthread_attr_init (&m_attr) == 0);
        if (m_joinable) {
            ATE_ASSERT(pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_JOINABLE) == 0);
        } else {
            ATE_ASSERT(pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED) == 0);
        }
    }

    // assume the destructor to be called by the creation thread.
    // otherwise m_joinable is not thread safe.
    Thread::~Thread() 
    {        
        if (m_joinable) {
            ATE_ASSERT(pthread_join(m_id, NULL) == 0);            
        }    
        pthread_attr_destroy(&m_attr);        
    }
    
    void Thread::start() 
    {
        ATE_ASSERT(pthread_create(&m_id, &m_attr, threadFunc, this) == 0);
    }
    
    // assume join() to be called by the creation thread.
    // otherwise m_joinable is not thread safe.
    void Thread::join()
    {
        if (m_joinable) {
            ATE_ASSERT(pthread_join(m_id, NULL) == 0);            
        }    
        m_joinable = false;
    }
    
    void Thread::sleep(unsigned int millis)
    {
        struct timespec req, rem;
        req.tv_sec = millis / 1000;
        req.tv_nsec = (millis % 1000) * 1000000;
        while (nanosleep(&req, &rem) != 0) {
            ATE_ASSERT(errno == EINTR);
            req = rem;    
        }
    }
    
    void SpinLock::lock()
    {
        unsigned int pauseCount = 1;
        unsigned int yieldCount = 0;
        unsigned int sleepCount = 1;
        
        while (true) {  
            if (m_lock.locked == 0) { // spin on dirty read, no lock on bus
                if (testAndSet(&m_lock.locked) == 0) {
                    return;
                }
            }
            // backoff
            if (pauseCount < pauseCountMax) {
                cpuPause(pauseCount);
                pauseCount = pauseCount << 1;
            } else if (yieldCount++ < yieldCountMax) { 
                // pause is so long, give other threads a chance
                Thread::yield();
            } else { // backoff by sleep after a few yields
                Thread::sleep(sleepCount);
                if (sleepCount < sleepCountMax)
                    sleepCount = sleepCount << 1;
            }            
        }
    }
    
    void SpinLock::unlock()
    {
        membar();
        m_lock.locked = 0;
    }
    
    Mutex::Mutex()
    {        
        ATE_ASSERT(pthread_mutexattr_init(&m_attr) == 0);
        ATE_ASSERT(pthread_mutex_init(&m_mutex, &m_attr) == 0);
    }
    
    Mutex::~Mutex()
    {                
        ATE_ASSERT(pthread_mutex_destroy(&m_mutex) == 0);
        pthread_mutexattr_destroy(&m_attr);
    }

    Condition::Condition()
    {        
        ATE_ASSERT(pthread_condattr_init(&m_condattr) == 0);
        ATE_ASSERT(pthread_cond_init(&m_cond, &m_condattr) == 0);
    }
    
    Condition::~Condition()
    {        
        ATE_ASSERT(pthread_cond_destroy(&m_cond) == 0);
        pthread_condattr_destroy(&m_condattr);
    }
} // namespace ate
