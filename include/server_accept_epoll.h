#ifndef SERVER_ACCEPT_EPOLL_H
#define SERVER_ACCEPT_EPOLL_H

#include <sys/socket.h> // for socket()
#include <sys/epoll.h> // for epoll_create1()
#include <netinet/in.h> // for sockaddr_in

#define MAX_EPOLL_EVENTS 1000
#define MAX_CONNECTIONS 1000


struct epoll_event buildEvent(int events, int fd);

void closeClient(int epollFd, int clientFd);
void addClient(int epollFd, int clientFd);

void acceptNewConnections(int epollFd, int socketServerFd);
char *readRequest(char *buffer, int epollFd, int *clientFd);
void waitAndAccept(int socketServerFd, struct sockaddr_in *socketAddress, socklen_t socketAddressLen);

#endif // SERVER_ACCEPT_EPOLL_H

