#include <ate/ate.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>  // TCP_NODELAY 
#include <netdb.h>        // getservbyname(), gethostbyname()
#include <fcntl.h>
#include <unistd.h>

static inline int Fcntl(int fd, int cmd, long arg)
{
    int flags;
    if ((flags = fcntl(fd, cmd, arg)) < 0) {
        ATE_ABORT("fcntl(): " << flags);
    }
    return flags;
}

static inline void Getsockopt(int s, int level, int optname,
                              void *optval, socklen_t *optlen)
{
    if (getsockopt(s, level, optname, optval, optlen) < 0) {
        ATE_ABORT("getsockopt() failed");
    }
}

static inline void Setsockopt(int s, int level, int optname,
                              const void *optval, socklen_t optlen)
{
    if (setsockopt(s, level, optname, optval, optlen) < 0) {
        ATE_ABORT("setsockopt() failed");
    }
}

namespace ate {
    IpAddress::IpAddress() {}
    IpAddress::~IpAddress() {}
    
    IpAddress IpAddress::fromString(const char* ipstr, Family family)
    {
        IpAddress ipaddr;
        int rc;
        ipaddr.m_family = family;
        if (ipaddr.m_family == IPV4) {
            rc = inet_pton(AF_INET, ipstr, &(ipaddr.m_addr.v4)); 
        } else { // IPV6
            rc = inet_pton(AF_INET6, ipstr, &(ipaddr.m_addr.v6));
        }
        ATE_ASSERT(rc > 0);
        strncpy0(ipaddr.m_ipstr, ipstr, sizeof(ipaddr.m_ipstr));
        return ipaddr;
    }

    IpAddress IpAddress::fromHost(const char* host)
    {
        IpAddress ipaddr;
        struct addrinfo hints, *res, *p;
        void* addr;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
        hints.ai_socktype = SOCK_STREAM;
        
        if (getaddrinfo(host, NULL, &hints, &res) != 0) {
            ATE_ABORT("getaddrinfo() host: " << host);
        }
        for(p = res;p != NULL; p = p->ai_next) {
            if (p->ai_family == AF_INET) { // IPv4
                addr = &(((struct sockaddr_in*)p->ai_addr)->sin_addr);
                memcpy(&ipaddr.m_addr.v4, addr, sizeof(struct in_addr));
                ipaddr.m_family = IpAddress::IPV4;
            } else { // IPv6
                addr = &(((struct sockaddr_in6 *)p->ai_addr)->sin6_addr);
                memcpy(&ipaddr.m_addr.v6, addr, sizeof(struct in6_addr));
                ipaddr.m_family = IpAddress::IPV6;
            }
            // convert the IP to a string and print it:
            if (inet_ntop(p->ai_family,addr, ipaddr.m_ipstr,
                          sizeof(ipaddr.m_ipstr)) == NULL) {
                ATE_ABORT("inet_ntop() failed");
            }
            break;
        }
        freeaddrinfo(res); // free the linked list
        return ipaddr;
    }

    void Socket::init(const IpAddress& ipaddr, int type, 
                      int rcvBufSz, int sendBufSz)
    {
        int domain;
        switch(ipaddr.family()) {
        case IpAddress::IPV4: 
            domain = PF_INET;  
            break;
        case IpAddress::IPV6:
            domain = PF_INET6;
            break;
        }
        if ((m_sockfd = ::socket(domain, type, 0)) < 0) {
            ATE_ABORT("failed to create stream socket. domain: " << domain);
        }
        std::cout << "socket created: " << m_sockfd << std::endl;
        this->setReuseAddress(true);
        // socket buffer size must be set before listen() or connect() 
        this->setRcvBufSize(rcvBufSz);
        this->setSendBufSize(sendBufSz);
    }

    void Socket::close()
    {
        if (m_sockfd > 0) {
            if (::close(m_sockfd) < 0) { // todo: log errno
                std::cerr << "socket close() failed: " << m_sockfd << std::endl;
                return;
            }
            std::cout << "socket closed: " << m_sockfd << std::endl;
            m_sockfd = -1;
        }
    }

    Socket::~Socket()
    { 
        close();
    }
    
    void Socket::setNonBlocking(bool b)
    {
        int flags;
        flags = Fcntl(m_sockfd, F_GETFL, 0);
        if (b) { // set to non-blocking
            Fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);
        } else { // set to blocking
            Fcntl(m_sockfd, F_SETFL, flags & (~O_NONBLOCK));
        }
    }

    bool Socket::getNonBlocking() const
    {
        int flags;
        flags = Fcntl(m_sockfd, F_GETFL, 0);
        return flags & O_NONBLOCK;
    }

    void Socket::setReuseAddress(bool b)
    {
        int opt = b ? 1 : 0;
        Setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }

    bool Socket::getReuseAddress() const
    {
        int opt;
        socklen_t optlen = sizeof(opt);
        Getsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, &optlen);
        return opt != 0;
    }

    void Socket::setRcvBufSize(int n)
    {
        Setsockopt(m_sockfd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n));
    }

    int Socket::getRcvBufSize() const
    {
        int opt;
        socklen_t optlen = sizeof(opt);
        Getsockopt(m_sockfd, SOL_SOCKET, SO_RCVBUF, &opt, &optlen);
        return opt;
    }

    void Socket::setSendBufSize(int n)
    {
        Setsockopt(m_sockfd, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n));
    }

    int Socket::getSendBufSize() const
    {
        int opt;
        socklen_t optlen = sizeof(opt);
        Getsockopt(m_sockfd, SOL_SOCKET, SO_SNDBUF, &opt, &optlen);
        return opt;
    }

    void Socket::setSockAddr(struct sockaddr& sa, socklen_t& addrlen,
                             const IpAddress& ipaddr, int port) 
    {
        switch(ipaddr.family()) {
        case IpAddress::IPV4: {
            addrlen = sizeof(sockaddr_in);
            sa.sa_family = AF_INET;
            struct sockaddr_in* sin = (struct sockaddr_in*)&sa;
            sin->sin_addr = ipaddr.m_addr.v4;
            sin->sin_port = htons(port);
            break;
        }
        case IpAddress::IPV6: {
            addrlen = sizeof(sockaddr_in6);
            sa.sa_family = AF_INET6;
            struct sockaddr_in6* sin6 = (struct sockaddr_in6*)&sa;
            sin6->sin6_addr = ipaddr.m_addr.v6;
            sin6->sin6_port = htons(port);
            break;
        }
        }
    }

    void Socket::setLocalIpAddr(IpAddress& ipaddr)
    {
        ipaddr.m_family = IpAddress::IPV4;
        ipaddr.m_addr.v4.s_addr = htonl(INADDR_ANY); // wildcard address
        strncpy0(ipaddr.m_ipstr, "*", sizeof(ipaddr.m_ipstr));
    }
    
    void Socket::setTcpNoDelay(bool b) {
        int opt = b ? 1 : 0;
        Setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    }

    bool Socket::getTcpNoDelay() const
    {
        int opt;
        socklen_t optlen = sizeof(opt);
        Getsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, &optlen);
        return opt != 0;
    }

    void Socket::setKeepAlive(bool b)
    {
        int opt = b ? 1 : 0;
        Setsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    }

    bool Socket::getKeepAlive() const
    {
        int opt;
        socklen_t optlen = sizeof(opt);
        Getsockopt(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, &optlen);
        return opt != 0;
    }    

    void TcpSocket::init(const IpAddress& ipaddr)
    {
        struct sockaddr sa;
        socklen_t addrlen;
        this->setSockAddr(sa, addrlen, ipaddr, 0);
        Socket::init(ipaddr, SOCK_STREAM, 
                     Socket::DEFAULT_BUF_SIZE, Socket::DEFAULT_BUF_SIZE);
        this->setTcpNoDelay(true);
        this->setKeepAlive(false);
        if (::bind(m_sockfd, &sa, addrlen) < 0) {
            ATE_ABORT("bind() failed: " << m_sockfd);
        }
    }

    TcpSocket::TcpSocket(bool noinit)
    {
        if (noinit) {
            return;
        }
        IpAddress ipaddr;
        Socket::setLocalIpAddr(ipaddr);
        this->init(ipaddr);
    }

    TcpSocket::TcpSocket(const char* localIp)
    {
        IpAddress ipaddr = IpAddress::fromString(localIp);
        this->init(ipaddr);
    }

    TcpSocket::TcpSocket(const IpAddress& localIp)
    {
        this->init(localIp);
    }

    int TcpSocket::connect(const IpAddress& remote, int port)
    {
        struct sockaddr sa;
        socklen_t addrlen;
        this->setSockAddr(sa, addrlen, remote, port);
        return ::connect(m_sockfd, &sa, addrlen);
    }

    int TcpSocket::connect(const char* remoteIp, int port)
    {
        IpAddress remote = IpAddress::fromString(remoteIp);
        return this->connect(remote, port);
    }
    
    size_t TcpSocket::readn(char* buf, size_t n)
    {
        size_t  nleft = n;
        ssize_t nread;
        while (nleft > 0) {
            if ((nread = read(m_sockfd, buf, nleft)) < 0) {
                if (errno == EINTR) {
                    continue;       // call read() again 
                } else {
                    ATE_ABORT("read failed: " << nread);
                }
            } else if (nread == 0) {
                break;              // EOF 
            }
            nleft -= nread;
            buf += nread;
        }
        return (n - nleft);   
    }

    size_t TcpSocket::writen(const char* buf, size_t n)
    {
        size_t  nleft = n;
        ssize_t nwritten;
        while (nleft > 0) {
            if ((nwritten = write(m_sockfd, buf, nleft)) <= 0) {
                if ((nwritten < 0 && errno == EINTR)) {
                    continue;       // call write() again
                } else {
                    ATE_ABORT("write failed: " << nwritten);
                }          
            }
            nleft -= nwritten;
            buf += nwritten;
        }
        return n;
    }

    void TcpServerSocket::init(const IpAddress& ipaddr, int port, int backlog)
    {
        struct sockaddr sa;
        this->setSockAddr(sa, m_addrlen, ipaddr, port);
        Socket::init(ipaddr, SOCK_STREAM, 
                     Socket::DEFAULT_BUF_SIZE, Socket::DEFAULT_BUF_SIZE);
        this->setTcpNoDelay(true);
        this->setKeepAlive(false);
        ATE_ASSERT(::bind(m_sockfd, &sa, m_addrlen) == 0);
        ATE_ASSERT(::listen(m_sockfd, backlog) == 0);
    }

    TcpServerSocket::TcpServerSocket(int port)
    {
        IpAddress ipaddr;
        Socket::setLocalIpAddr(ipaddr);
        int backlog = 128;
        this->init(ipaddr, port, backlog);
    }

    TcpServerSocket::TcpServerSocket(const IpAddress& localIp, int port, 
                                     int backlog)
    {
        this->init(localIp, port, backlog);
    }

    int TcpServerSocket::accept(TcpSocket& clientSocket)
    {
        struct sockaddr cliaddr;
        int connfd;
        while (true) {
            if ((connfd = ::accept(m_sockfd, &cliaddr, &m_addrlen)) < 0) {
                if (errno == EINTR)
                    continue;       /* back to for() */
                else
                    ATE_ABORT("accept error");
            }
            break;
        }
        clientSocket.setSockfd(connfd);
        std::cout << "socket accepted: " << connfd << std::endl;        
        return connfd;
    }
}
