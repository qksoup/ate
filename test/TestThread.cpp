#include <ate/ate.hpp>
#include <unistd.h>

using namespace std;
using namespace ate;

namespace {
    int count = 0;
    SpinLock lock;
    Monitor wlock;
    bool ready = false;

    class MyThread : public ate::Thread {
    public:
        MyThread(bool b) : Thread(b) {
        }

        void run() {
            cout << id() << " running" << endl;
            for (int i = 0; i < 10000; ++i) {
                lock.lock();
                cout << id() << " locked - count:" << count++ << endl;
                lock.unlock();
                cout << id() << " unlocked " << endl;
                Thread::yield();
            }
        }
    };

    #define mylog(msg) \
    {\
        cout << ate::MicroTime::now(); \
        cout << "[" << Thread::currentId()  << "]" << msg << endl; \
     }
    
    class Thread1 : public ate::Thread {
    public:
        Thread1() : Thread(true) {}
        void run() {            
            for (int i = 0; i < 10; ++i) {
                ate::Thread::sleep(500);                
                mylog("sleeping");
            }
            {
                LockGuard<Monitor> guard(wlock);
                ready = true;   
                wlock.notify();
            }
            mylog("is ready now. thread 1 exiting");
        }
    };
    
    class Thread2 : public ate::Thread {
    public:
        Thread2() : Thread(true) {}
        void run() {        
            MicroTime now;        
            wlock.lock();
            while (!ready) {
                mylog("waiting for ready");
                now = ate::MicroTime::now().add(500);
                if (wlock.timedWait(now) == ETIMEDOUT)
                    mylog("cond wait timed out");
            }
            wlock.unlock();
            mylog("see ready now. thread2 exiting");
        }
    };
    
    void testThread12()
    {
        Thread1 t1;
        Thread2 t2;
        t1.start();
        t2.start();
        t1.join();
        t2.join();
    }
    
    void testThread()
    {
        const int tc = 2;
        MyThread* thread[tc];
        for (int i = 0; i < tc; ++i) {
            MyThread* t = new MyThread(true);
            thread[i] = t;
            cout << "joinable:" << t->joinable() << endl;
            t->start();
        }
    
        for (int i = 0; i < tc; ++i) {
            thread[i]->join();
        }
    }
    
    void testSignal()
    {
        SignalManager::init();
        pid_t pid = fork();
        if (pid == 0) { //  Child process:    
            cout << "child process created. pid - " << getpid() << endl;
            exit(0);
        } else if (pid > 0) { // Parent process:
            cout << "parent process pid - " << getpid() << endl;
            while (true) {
                Thread::sleep(1000);
            }
        }        
    }
    
    void testMutex()
    {
        Mutex mutex;
    }
    
    class SimpleHandler : public EventHandler {
    public:
        SimpleHandler() : i(-1) {}
        virtual void onEvent(Event& event) {
            if (event.type > i && event.type - i != 1) 
                abort();
            i = event.type;
            snprintf(m_buf, sizeof(m_buf), "processing event %d", i); 
            mylog(m_buf);
        }
    private:
        int i;
        char m_buf[1024];
    };
    SimpleHandler handler;

    bool timercb(const ate::MicroTime&, void*)
    {
        using ate::EventLoop;
        using ate::Event;
        mylog("calling timercb");
        return false; // done
    }

    bool stopLoop(const ate::Timer& timer)
    {
        using ate::EventLoop;
        using ate::Event;
        EventLoop* evloop = static_cast<EventLoop*>(timer.aux);
        evloop->setRunning(false);
        return true; // done
    }

    class ContextThread : public Thread {
    public:
        ContextThread(EventLoop& evloop)
            : Thread(true), m_evloop(evloop), m_added(false) {}
        void run() {            
            int n;
            Event event;
            while (m_evloop.isRunning()) { // not thread safe
                //n = rand() % 4096 + 1; 
                n = 1024;
                mylog("adding " << n << " events");
                for (int i = 0; i <n; ++i) {                    
                    event.type = i;
                    event.handler = &handler;
                    m_evloop.push(event);
                    mylog("added " << i << "th event");
                }
                
                Thread::sleep(60);
                if (!m_added) {
                    int millis = 6000;
                    Timer timer;
                    timer.time = MicroTime::now().add(millis);
                    timer.increment = -1;
                    timer.tcb = stopLoop;
                    timer.aux = &m_evloop;
                    m_evloop.addTimer(timer);
                    mylog("added timer " << millis);
                    m_added = true;
                }
            }
        }
    private:
        EventLoop& m_evloop;
        bool m_added;
    };

    class MyEventLoop : public EventLoop {
    public:
        virtual void begin() { mylog("begin cycle"); }
        virtual void end() { mylog("end cycle"); }
    };    

    void testEventLoop()
    {
        MyEventLoop evloop;
        ContextThread ctxThread(evloop);
        ctxThread.start();
        evloop.run();
        mylog("evloop stopped");
        ctxThread.join();
    }

    void testWait()
    {

        MicroTime time;
        time.setMin();
        Monitor lock;
        lock.lock();
        int rc = lock.timedWait(time);
        ATE_ASSERT(rc == ETIMEDOUT);
        lock.unlock();
        MicroTime start = MicroTime::now();
        uint64_t id;
        for (int i = 0; i < 10000; ++i) {
            id = pthread_self();
        }
        MicroTime end = MicroTime::now();
        cout  << id << " start:" << start << " end:" << end << endl;

        start = MicroTime::now();
        timeval tv;
        for (int i = 0; i < 1000000; ++i) {
            gettimeofday(&tv, NULL);
        }
        end = MicroTime::now();
        cout << id << " start:" << start << " end:" << end << endl;
    }
}

int main() 
{
    //testThread12();
    //testSignal();
    testEventLoop();
    //testWait();
    mylog("exiting...");
}
