#include <arpa/inet.h>  // for inet_ntoa()
#include <netdb.h>      // for getaddrinfo(),  getservbyname, gethostbyname()
#include <sys/socket.h> // for socket()
#include <unistd.h>     // for close()
#include <netinet/tcp.h> // for TCP_NODELAY
#include <time.h>        // for time()

#include "../lib/color/color.h"
#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
//#include "accept_client_epoll.h"
//#include "accept_client_fork.h"
//#include "accept_client_thread.h"
#include "accept_client_thread_epoll.h"
#include "helper.h"
#include "server.h"

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

   /*  if (OPTIONS.TCPKeepAlive) {
        makeTCPKeepAlive(socketServerFd);
    } */

    // https://baus.net/on-tcp_cork/
    int enableTCP_NO_DELAY = 1;
    if (setsockopt(socketServerFd, IPPROTO_TCP, TCP_NODELAY, &enableTCP_NO_DELAY, sizeof(enableTCP_NO_DELAY)) == -1) {
        die("setsockopt TCP_NODELAY");
    }

    int enableTCP_CORK = 1;
    if (setsockopt(socketServerFd, IPPROTO_TCP, TCP_CORK, &enableTCP_CORK, sizeof(enableTCP_CORK)) == -1) {
        die("setsockopt TCP_CORK");
    }

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

    // SOMAXCONN: Maximum connections queue (not accepted yet)
    // cat /proc/sys/net/core/somaxconn
    if (listen(socketServerFd, 4096) == -1) {
        die("listen");
    }

    printf(GREEN "Server listening on http://%s:%d ..." RESET "\n\n", inet_ntoa(socketAddress.sin_addr), htons(socketAddress.sin_port));

    acceptClientsThreadEpoll(socketServerFd);
    //acceptClientsThread(socketServerFd);
    //acceptClientsEpoll(socketServerFd);
    //acceptClientsFork(socketServerFd);

    close(socketServerFd);
}

void makeTCPKeepAlive(int socketServerFd) {
    int enableKeepAlive = 1;
    if (setsockopt(socketServerFd, SOL_SOCKET, SO_KEEPALIVE, &enableKeepAlive, sizeof(enableKeepAlive)) == -1) {
        die("ERROR: setsocketopt(), SO_KEEPALIVE");
    }

    int keepcnt = KEEP_ALIVE_TCP_KEEPCNT;
    if (setsockopt(socketServerFd, SOL_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt))) {
        die("ERROR: setsocketopt(), TCP_KEEPCNT");
    }

    int idle = KEEP_ALIVE_TCP_KEEPIDLE;
    if (setsockopt(socketServerFd, SOL_TCP, TCP_KEEPIDLE, &idle, sizeof(idle))) {
        die("ERROR: setsocketopt(), TCP_KEEPIDLE");
    }

    int interval = KEEP_ALIVE_TCP_KEEPINTVL;
    if (setsockopt(socketServerFd, SOL_TCP, TCP_KEEPINTVL, &interval, sizeof(interval))) {
        die("ERROR: setsocketopt(), TCP_KEEPINTVL");
    }
}