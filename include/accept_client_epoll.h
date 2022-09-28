#ifndef ACCEPT_CLIENT_EPOLL_H
#define ACCEPT_CLIENT_EPOLL_H

#include <netinet/in.h> // for sockaddr_in
#include <sys/epoll.h>  // for epoll_create1()
#include <sys/socket.h> // for socket()

#include "queue_connections.h"

void handleEpollFacade(int socketServerFd);// facade for one epollFd per thread
void handleEpoll(int socketServerFd, int epollFd, struct epoll_event *events, struct QueueConnectionsType *queueConnections);
void acceptEpollConnection(int epollFd, int socketServerFd, int events);

struct epoll_event buildEpollEvent(int events, int fd);
void addEpollClient(int epollFd, int clientFd, int events);
void modEpollClient(int epollFd, int clientFd, int events);
void closeEpollClient(int epollFd, int clientFd);

#endif // ACCEPT_CLIENT_EPOLL_H
