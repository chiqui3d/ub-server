#ifndef ACCEPT_CLIENT_EPOLL_H
#define ACCEPT_CLIENT_EPOLL_H

#include <netinet/in.h> // for sockaddr_in
#include <sys/epoll.h>  // for epoll_create1()
#include <sys/socket.h> // for socket()

#define MAX_EPOLL_EVENTS 1000

void acceptClients(int socketServerFd, struct sockaddr_in *socketAddress, socklen_t socketAddressLen);

void acceptConnection(int epollFd, int socketServerFd);

struct epoll_event buildEvent(int events, int fd);

void addClient(int epollFd, int clientFd);

void closeClient(int epollFd, int clientFd);

#endif // ACCEPT_CLIENT_EPOLL_H
