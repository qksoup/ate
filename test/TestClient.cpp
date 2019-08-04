#include <ate/ate.hpp>

using namespace std;
using namespace ate;

int main(int argc, char* argv[])
{
    int port;
    if (argc > 1) {
        port = atoi(argv[1]);
    } else {
        port = 2000;
    }
    TcpSocket socket;
    IpAddress ip = IpAddress::fromHost("localhost");
    int rc = socket.connect(ip, port);
    ATE_ASSERT(rc == 0);
    ATE_ASSERT(!socket.getNonBlocking());
    ATE_ASSERT(socket.getReuseAddress());
    cout << "socket rcvbuf: " << socket.getRcvBufSize() << endl;
    cout << "socket sendbuf: " << socket.getSendBufSize() << endl;
    char buf[1024];
    int n;
    std::cout << "sent: \n";
    for (int i = 0; i < 10; ++i) {
        n = snprintf(buf, sizeof(buf), "hello-%02d|", i);
        socket.writen(buf, n);
        std::cout << buf;
    }
    std::cout << endl;
    socket.close();
}
