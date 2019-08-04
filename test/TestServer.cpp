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
    TcpServerSocket svrsocket(port);
    ATE_ASSERT(!svrsocket.getNonBlocking());
    ATE_ASSERT(svrsocket.getReuseAddress());
    TcpSocket socket(true); // noinit
    ATE_ASSERT(svrsocket.accept(socket) > 0);
    char buf[1024];
    int n;
    cout << "socket tcp no delay: " << socket.getTcpNoDelay() << endl;
    cout << "socket keep alive: " << socket.getKeepAlive() << endl;
    cout << "socket rcvbuf: " << socket.getRcvBufSize() << endl;
    cout << "socket sendbuf: " << socket.getSendBufSize() << endl;
    cout << "server received: " << endl;
    do {
        n = socket.readn(buf, sizeof(buf));
        for (int i = 0; i < n; ++i)
            cout << buf[i];
    } while (n > 0);
    cout << endl;
    socket.close();
    svrsocket.close();
}
