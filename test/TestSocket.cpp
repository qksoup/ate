#include <ate/ate.hpp>

using namespace std;
using namespace ate;

int main(int argc, char* argv[])
{
    const char* host;
    if (argc == 1)
        host = "www.google.com";
    else 
        host = argv[1];
    IpAddress ip = IpAddress::fromHost(host);
    IpAddress ip2 = IpAddress::fromString(ip.toString());
    cout << host << " " << ip.toString() << endl;
    cout << "ip2: " << ip2.toString() << endl;
    cout << "ip address family:" << ip.family() << endl;

    TcpSocket socket;
    ATE_ASSERT(!socket.getNonBlocking());
    ATE_ASSERT(socket.getReuseAddress());
    cout << "rcv buf size: " << socket.getRcvBufSize() << endl;
    cout << "send buf size: " << socket.getSendBufSize() << endl;
    int rc = socket.connect(ip, 80);
    ATE_ASSERT(rc == 0);
    socket.setTcpNoDelay(true);
    char buf[1024];
    const char* req = "GET index.html HTTP/1.0\r\n"
        "Accept: text/plain, text/html, text/*\r\n"
        "\r\n";

    socket.writen(req, strlen(req));
    int n;
    do {
        n = socket.readn(buf, sizeof(buf));
        for (int i = 0; i < n; ++i)
            cout << buf[i];
    } while (n > 0);
    cout << endl;
}
