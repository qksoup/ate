#ifndef ATE_HPP
#define ATE_HPP

// linux C headers
#include <arpa/inet.h>
#include <sys/time.h>
#include <sched.h>
#include <pthread.h>
// standard C headers
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// standard C++ headers
#include <iostream>
#include <sstream>
#include <vector>

#define ATE_ABORT(msg) {                                                \
        int errnum = errno;                                             \
        char buf[256];                                                  \
        std::stringstream ss;                                           \
        ss << __FILE__ << ':' << __LINE__ << ' ' << msg;                \
        std::cerr << ss.str() << std::endl;                             \
        if (errnum != 0) {                                              \
            char* errmsg = strerror_r(errnum, buf, sizeof(buf));        \
            std::cerr << "sys error: " << errmsg << std::endl;          \
        }                                                               \
        ::abort();                                                      \
    }

#define ATE_ASSERT(expr) {                      \
        if (!(expr)) {                          \
            ATE_ABORT(#expr " assert failed");  \
        }                                       \
    }
namespace ate {  
    union Alignment {
        double d;
        void* v;
    };

    /**
     * align() works only with power of 2 blocks and 
     * assumes 2's complement arithmetic.
     */
    inline size_t align(size_t n) 
    {
        return (n + sizeof(union Alignment) - 1) & 
            ~(sizeof(union Alignment) - 1);
    }
    
    class Region {
    public:
        union Link {
            Link* next;
            Alignment a;
        };
        Region();
        ~Region();
        /// returns a pointer to memory of size n bytes
        void* malloc(size_t n);
    private:
        Link* m_head;
    };
        
    /**
     * Bin can be used for memory that is released when the program exits.
     */
    class Bin {
    public:
        Bin();
        ~Bin();
        void* alloc(size_t n, bool zero = true);
        char* strdup(const char* str);
        template<typename T> inline
        T* create() { return new(alloc(sizeof(T))) T; }
    private:
        Region m_region;
        char* m_cur;
        char* m_end;
        static const size_t s_size;
    };

    /**
     * Pool allocates memory for objects of the same type. 
     */
    template <typename T>
    class Pool {
    public:
        union Block {
            Block* next;
            char mem[sizeof(T)];
        };
        Pool(const char* name = "Pool", 
             size_t initBlocks = 128, 
             size_t maxIncrement = 1024);
        /// return a pointer to a T object
        T* alloc();
        /// put memory back to free list. ptr has to be from Pool.alloc() 
        inline void dealloc(T* ptr);
        /// destructor releases all allocated memory 
        ~Pool() {}
    private:
        void grow(size_t nBlocks);
        const char* m_name;
        Region m_region;
        Block* m_head;
        size_t m_maxIncr;
        size_t m_size;
    };

    /// microseconds since 1970/01/01 stored in 64-bit integer. 
    struct MicroTime {
        uint64_t microsec; 
        
        MicroTime(): microsec((uint64_t)-1) {}
        void setMax() { microsec = (uint64_t)-1; }
        void setMin() { microsec = 0; }
        bool isMax() const { return microsec == (uint64_t)-1; }
        bool isMin() const { return microsec == 0; }
        inline MicroTime& add(int millisec) {
            microsec += millisec * 1000;
            return *this;
        }
        
        static MicroTime now() {
            struct timeval tv;
            MicroTime mt;
            if (gettimeofday(&tv, NULL) != 0)
                ATE_ABORT("gettimeofday() failed");        
            mt.microsec = ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;
            return mt;
        }
        
    };

    class Thread {
    public: 
        Thread(bool joinable = false);
        virtual ~Thread();
        
        void start();
        virtual void run() = 0;
        void join();
        bool joinable() { return m_joinable; }
        pthread_t id() { return m_id; }
        
        static void sleep(unsigned int millis); 
        inline static void yield() { ATE_ASSERT(sched_yield() == 0); }
        inline static pthread_t currentId() { return pthread_self(); }
    private:
        pthread_attr_t m_attr;
        pthread_t m_id;
        bool m_joinable;
    };

    class SpinLock {
    public:
        SpinLock() { m_lock.locked = 0; }
        ~SpinLock() {}
        void lock();
        void unlock();
    private:
        volatile union { 
            volatile int locked;
            union Alignment padding;  // word tearing
        } m_lock;
    };

    class Mutex {
    public:
        friend class Condition;
        Mutex();
        ~Mutex();
        void lock() { ATE_ASSERT(pthread_mutex_lock(&m_mutex) == 0); }
        void unlock() { ATE_ASSERT(pthread_mutex_unlock(&m_mutex) == 0); }
    private:
        pthread_mutexattr_t m_attr;
        pthread_mutex_t m_mutex;
    };
    
    template <typename Lockable>
    class LockGuard {
    public:
        LockGuard(Lockable& lock) : m_lock(lock) { m_lock.lock(); }        
        ~LockGuard() { m_lock.unlock(); }
    private:
        Lockable& m_lock;
    };
    
    // Condition Variable 
    class Condition {
    public:
        Condition();        
        ~Condition();        
        int wait(Mutex& mutex) {            
            ATE_ASSERT(pthread_cond_wait(&m_cond, &mutex.m_mutex) == 0);
            return 0; // always return 0
        }
        int timedWait(Mutex& mutex, const MicroTime& mt) {
            if (mt.isMax())
                return wait(mutex);
            struct timespec abstime;
            abstime.tv_sec = mt.microsec / 1000000; 
            abstime.tv_nsec = (mt.microsec % 1000000) * 1000;
            int rc = pthread_cond_timedwait(&m_cond, &mutex.m_mutex, &abstime);
            ATE_ASSERT(rc == 0 || rc == ETIMEDOUT);
            return rc; // todo: enum timeout
        }
        void notify() { ATE_ASSERT(pthread_cond_signal(&m_cond) == 0); }
        void notifyAll() { ATE_ASSERT(pthread_cond_broadcast(&m_cond) == 0); }
    private:
        pthread_condattr_t m_condattr;
        pthread_cond_t m_cond;
    };
    
    // Monitor = mutex + conditional variable
    class Monitor {
    public:
        Monitor() {}
        ~Monitor() {}
        void lock() { m_mutex.lock(); }
        void unlock() { m_mutex.unlock(); }
        int wait() { return m_cond.wait(m_mutex); }
        int timedWait(const MicroTime& absTime) { 
            return m_cond.timedWait(m_mutex, absTime);
        }
        void notify() { m_cond.notify(); }
        void notifyAll() { m_cond.notifyAll(); }
    private:
        Mutex m_mutex;
        Condition m_cond;
    };
    
    /// SignalManager is a singleton. 
    /// init() should be called at the beginning of main thead. 
    /// It will block asynchronous signals and creates a thread to 
    /// sigwait() asynchronous signals interested.     
    class SignalManager {
    public: 
        /// to be called in the main thread before other threads are created
        static void init();
    private:
        SignalManager();
    };

    /**
     * return true if the timer callback is done, otherwise not done(recurrent)
     */
    struct Timer;
    typedef bool (*TimerCallback)(const Timer& timer);    
    struct Timer {
        MicroTime time;
        int increment; // in milliseconds         
        TimerCallback tcb;
        void* aux;
    };

    // Timer priority queue, it is suitable for small number of timers. 
    class TimerQueue {
    public:        
        TimerQueue(const char* name = "TimerQueue")
            : m_name(name), m_pool(m_name) {
            m_dummy.prev = m_dummy.next = &m_dummy; 
        }
        const MicroTime& minTime() const { return m_dummy.next->timer.time; }
        void add(const Timer& timer);
        void dispatch(const MicroTime& now);
    private:
        struct TimerNode {
            TimerNode* next;
            Timer timer;
            TimerNode* prev;
        };
        const char* m_name;
        Pool<TimerNode> m_pool;
        TimerNode m_dummy; // dummy node for both head and tail         
        void doAdd(TimerNode* newNode);
    };
    
    // FastQueue is for one/multiple producer, ONE consumer
    class EventLoop;
    template<typename T>
    class FastQueue {
    public:
        struct Node {
            Node* next; // have to be first since it will be Pool<T>::Block
            T obj;
        };
        friend class EventLoop;
        FastQueue(Monitor& monitor, const char* name = "FastQueue", 
                  size_t initNodes = 128, size_t maxIncrement = 1024) 
            : m_monitor(monitor), m_name(name), m_maxIncr(maxIncrement),
              m_size(0), m_wtail(&m_whead), m_rhead(NULL), m_rtail(NULL) {
            grow(initNodes);
        }
        void push(const T& elem);
        inline Node* getWork(const MicroTime& abstime);        
    private:
        Monitor& m_monitor; 
        const char* m_name;
        Region m_region;
        size_t m_maxIncr;
        size_t m_size;
        Node m_whead; // always has a head node for write queue
        Node* m_wtail;
        Node* m_rhead;
        Node* m_rtail;
        MicroTime m_waitTime;
        void grow(size_t nNodes); 
        Node* getWorkWithLock(const MicroTime& abstime); 
    };
    
    class EventHandler;
    struct Event {
        int type;
        int len;
        void* data;
        EventHandler* handler; // make sure this is not NULL when created
    };
    
    class EventHandler {
    public:
        virtual void onEvent(Event& event) = 0;
        virtual ~EventHandler() {}
    };
    typedef FastQueue<Event> EventQueue; 
    typedef EventQueue::Node EventNode;
    
    class EventLoop {
    public:
        EventLoop();
        virtual ~EventLoop() {}
        void push(const Event& event);
        void addTimer(const Timer& timer);
        void run(int millis = 0);
        void setRunning(bool b) { m_running = b; }
        bool isRunning() { return m_running; }
        virtual void begin() {}
        virtual void end() {}        
    private:
        Monitor m_monitor;
        bool m_running;
        TimerQueue m_timers;
        EventQueue m_evq;
        EventNode* getWork(const MicroTime& now, bool& waited);
    };

    // string utilities: it is provided for convenience, not performance
    char* trimLeft(char* str, const char* delim = " \f\n\r\t\v");
    char* trimRight(char* str, const char* delim = " \f\n\r\t\v");
    char* trim(char* str, const char* delim = " \f\n\r\t\v");
    
    void toLower(char* str);
    void toUpper(char* str);
    
    /**
     * strncpy0 = strncpy + null terminated guarantee
     */
    inline char* strncpy0(char *dest, const char *src, size_t n) 
    {
        strncpy(dest, src, n);
        dest[n - 1] = '\0';
        return dest;
    }

    /** 
     * tokenize() stores tokens in the vector. No empty token("") is returned.
     * Warning: tokenize() modifies the input string.
     */
    void tokenize(std::vector<char*>& tokens, char* str,
                  const char* delim = " \f\n\r\t\v");
    /**
     * split() stores tokens in the vector. Empty token("") will be returned 
     * if there are consecutive delimiters or the string starts with delimiter.
     * Warning: tokenize() modifies the input string.
     */
    void split(std::vector<char*>& tokens, char* str, 
               const char* delim = " \f\n\r\t\v");
    
    // socket wrappers
    class IpAddress {
    public:
        friend class Socket;
        friend class TcpSocket;
        friend class TcpServerSocket;
        friend class UdpSocket;
        enum Family {
            IPV4 = 0,
            IPV6
        };
        virtual ~IpAddress();
        Family family() const { return m_family; } 
        const char* toString() const { return m_ipstr; };

        static IpAddress fromString(const char* ipstr, Family family = IPV4);
        // return the first ip address for the host
        static IpAddress fromHost(const char* host); 
    protected:
        IpAddress();
        Family m_family;
        char m_ipstr[INET6_ADDRSTRLEN];
        union {
            struct in_addr v4;
            struct in6_addr v6;
        } m_addr;
    };

    class Socket {
    public:
        enum {
            DEFAULT_BUF_SIZE = 2 * 1024 * 1024 // 2 MB
        };
        Socket() : m_sockfd(-1) { }
        int getSockfd() const { return m_sockfd; }
        void setSockfd(int sockfd) { m_sockfd = sockfd; }
        void setNonBlocking(bool b);
        void setReuseAddress(bool b);
        void setRcvBufSize(int n);
        void setSendBufSize(int n);
        bool getNonBlocking() const;
        bool getReuseAddress() const; 
        int getRcvBufSize() const;
        int getSendBufSize() const;
        // TCP only options
        void setTcpNoDelay(bool b);
        bool getTcpNoDelay() const;
        void setKeepAlive(bool b);
        bool getKeepAlive() const;
        void close();
    protected:
        virtual ~Socket() = 0;
        void setSockAddr(struct sockaddr& sa, socklen_t& addrlen,
                         const IpAddress& ipaddr, int port);
        void setLocalIpAddr(IpAddress& ipaddr);
        void init(const IpAddress& ipaddr, int type, 
                  int rcvBufSz, int sendBufSz);
        int m_sockfd;        
    };
    
    class TcpSocket : public Socket {
    public:
        TcpSocket(bool noinit = false);
        TcpSocket(const char* localIp);
        TcpSocket(const IpAddress& localIp);
        int connect(const IpAddress& remote, int port);
        int connect(const char* remoteIp, int port);
        size_t readn(char* buf, size_t n);
        size_t writen(const char* buf, size_t n);
    protected:
        void init(const IpAddress& ipaddr);
    };
    
    class TcpServerSocket : public Socket {
    public:
        TcpServerSocket(int port);
        TcpServerSocket(const IpAddress& localIp, int port, int backlog);
        int accept(TcpSocket& clientSocket);
    private:
        void init(const IpAddress& ipaddr, int port, int backlog);
        socklen_t m_addrlen;
    };

    class UdpSocket : public Socket {
    public:
        UdpSocket(unsigned short port);
        void setBroadcast (bool b);
        void setMulticastTtl (int ttl);
        void setLoopback(bool b = true);
        void joinGroup(const IpAddress& group);
        void leaveGroup(const IpAddress& group);
    };
    
} // namespace ate
#include <ate/arch/Assembly.hpp>

//////////////////////////////////////////////////////////////////////////////
// template implementations
//////////////////////////////////////////////////////////////////////////////
namespace ate {
    template<typename T>
    Pool<T>::Pool(const char* name, size_t initBlocks, size_t maxIncrement) 
        : m_name(name), m_head(NULL), m_maxIncr(maxIncrement), m_size(0)
    {
        if (initBlocks == 0)
            initBlocks = 128;
        if (m_maxIncr == 0)
            m_maxIncr = 1024;
        grow(initBlocks); // post condition: m_size = initNodes
    }

    template<typename T>
    T* Pool<T>::alloc() 
    {
        if (m_head == NULL) {
            size_t increment = m_size < m_maxIncr ? 
                m_size : m_maxIncr;
            grow(increment);
        }
        T* ptr = reinterpret_cast<T*>(m_head->mem);
        m_head = m_head->next;        
        return ptr;
    }
   
    template<typename T>
    void Pool<T>::grow(size_t nBlocks) 
    {
        Block* first;
        Block* cur;
        first = cur = static_cast<Block*> 
            (m_region.malloc(sizeof(Block) * nBlocks));
        for (size_t i = 0; i < nBlocks - 1; ++i) {
            cur->next = cur + 1;
            ++cur;
        }
        cur->next = m_head; // the list ends with NULL
        m_head = first;
        m_size += nBlocks;
        std::cout << m_name << " capacity: " << m_size << std::endl;
    }
    
    template<typename T> inline
    void Pool<T>::dealloc(T* ptr)
    {
        Block* block = reinterpret_cast<Block*>(ptr);
        block->next = m_head;
        m_head = block;
    }
    
    template<typename T>
    void FastQueue<T>::push(const T& elem) 
    {
        LockGuard<Monitor> guard(m_monitor);
        bool empty = (m_wtail == &m_whead); // empty queue
        Node* newNode;
        if (m_wtail->next == NULL) { // no more free nodes
            size_t increment = m_size < m_maxIncr ? m_size : m_maxIncr;
            grow(increment);
        } 
        // now we have some free nodes
        newNode = m_wtail->next;
        new(&newNode->obj) T(elem); // use copy constructor to copy value
        m_wtail = m_wtail->next;
        if (empty) 
            m_monitor.notify();        
    }
    
    template<typename T> inline typename FastQueue<T>::Node* 
    FastQueue<T>::getWork(const MicroTime& abstime)
    {
        LockGuard<Monitor> guard(m_monitor);
        return getWorkWithLock(abstime);
    }
    
    template<typename T>
    void FastQueue<T>::grow(size_t nNodes) 
    {
        Node* first;
        Node* cur;
        first = cur = static_cast<Node*>
            (m_region.malloc(sizeof(Node) * nNodes));
        for (size_t i = 0; i < nNodes - 1; ++i) {
            cur->next = cur + 1;
            ++cur;
        }
        cur->next = NULL; // the list ends with NULL
        m_wtail->next = first;
        m_size += nNodes;
        std::cout << m_name << " capacity: " << m_size << std::endl;
    }
    
    template<typename T> typename FastQueue<T>::Node* 
    FastQueue<T>::getWorkWithLock(const MicroTime& abstime)
    {
        if (m_rhead != NULL) { // free read queue memory first 
            Node* tmp = m_wtail->next;
            m_wtail->next = m_rhead;
            m_rtail->next = tmp;
            m_rhead = m_rtail = NULL;
        }
        m_waitTime = abstime;
        while (m_wtail == &m_whead) { // no item in the queue yet
            std::cout << "event queue is empty, waiting..." << std::endl;
            if (m_monitor.timedWait(m_waitTime) == ETIMEDOUT) 
                return NULL;
        }
        m_rhead = m_whead.next;
        m_rtail = m_wtail;
        m_whead.next = m_wtail->next;
        m_rtail->next = NULL;
        // reset write queue tail
        m_wtail = &m_whead;
        return m_rhead;
    } 
} 

// global implementations
inline bool operator==(const ate::MicroTime& t1, const ate::MicroTime& t2)
{
    return t1.microsec == t2.microsec;
}

inline bool operator!=(const ate::MicroTime& t1, const ate::MicroTime& t2)
{
    return t1.microsec != t2.microsec;
}

inline bool operator<(const ate::MicroTime& t1, const ate::MicroTime& t2)
{
    return t1.microsec < t2.microsec;
}

inline bool operator>(const ate::MicroTime& t1, const ate::MicroTime& t2)
{
    return t1.microsec > t2.microsec;
}

inline bool operator>=(const ate::MicroTime& t1, const ate::MicroTime& t2)
{
    return t1.microsec >= t2.microsec;
}

inline bool operator<=(const ate::MicroTime& t1, const ate::MicroTime& t2)
{
    return t1.microsec <= t2.microsec;
}

inline std::ostream& operator<<(std::ostream& os, const ate::MicroTime& ts)
{
    uint64_t v = ts.microsec;
    int microsec = v % 1000000;  v /= 1000000;
    int sec = v % 60; v /= 60;
    int min = v % 60; v /= 60;
    int hour = v % 24; 
    char buf[16];
    snprintf(buf, sizeof(buf), "%02u%02u%02u.%06u", hour, min, sec, microsec);
    return os << buf;
}

#endif // ATE_HPP
