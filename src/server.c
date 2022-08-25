#include <arpa/inet.h>  // for inet_ntoa()
#include <netdb.h>      // for getaddrinfo(),  getservbyname, gethostbyname()
#include <sys/socket.h> // for socket()
#include <unistd.h>     // for close()
// #include <netinet/in.h> // for sockaddr_in
#include <fcntl.h>       // for fcntl() nonblocking socket
#include <netinet/tcp.h> // for TCP_NODELAY
#include <time.h>        // for time()

#include "../lib/color/color.h"
#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "server.h"
#include "server_accept_epoll.h"

int makeSocketNonBlocking(int fd) {
    int flags, s;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(fd, F_SETFL, flags);
    if (s == -1) {
        perror("fcntl");
        return -1;
    }

    return 0;
}

void serverRun(struct Options options) {

    int socketServerFd = socket(PF_INET, SOCK_STREAM, 0);

    if (socketServerFd == -1) {
        die("socket");
    }

    int enable = 1;
    // avoid "Address already in use" error message (reuse port)
    if (setsockopt(socketServerFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof enable) == -1) {
        die("setsockopt SO_REUSEADDR");
    }

    /*     struct timeval timeout;
        timeout.tv_sec  = 3;  // after 7 seconds connect() will timeout
        timeout.tv_usec = 0;
        setsockopt(socketServerFd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)); */

    /*  int enableTCP= 1;
    // disable Nagle since we buffer everything ourselves
    if (setsockopt(socketServerFd, IPPROTO_TCP, TCP_NODELAY, &enableTCP, sizeof enableTCP) == -1) {
            die("setsockopt TCP_NODELAY");
        }
    */

    struct hostent *localHostName = gethostbyname(options.address);
    if (localHostName == NULL) {
        die("gethostbyname %s failed", options.address);
    }
    struct in_addr **addr_list = (struct in_addr **)localHostName->h_addr_list;
    struct in_addr *addr = addr_list[0];

    struct sockaddr_in socketAddress = {0};
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(options.port);

    // socketAddress.s_addr = INADDR_ANY; // bind to any address on this machine
    socketAddress.sin_addr = *addr;
    socklen_t socketAddressLen = sizeof(socketAddress);

    if (bind(socketServerFd, &socketAddress, socketAddressLen) == -1) {
        die("bind");
    }

    makeSocketNonBlocking(socketServerFd);

    // SOMAXCONN: Maximum connections queue
    if (listen(socketServerFd, SOMAXCONN) == -1) {
        die("listen");
    }

    consoleDebug(GREEN "Server listening on %s:%d ..." RESET "\n\n", inet_ntoa(socketAddress.sin_addr), htons(socketAddress.sin_port));

    waitAndAccept(socketServerFd, &socketAddress, socketAddressLen);

    close(socketServerFd);
}