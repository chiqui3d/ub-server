#ifndef ACCEPT_CLIENT_FORK_H
#define ACCEPT_CLIENT_FORK_H

#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket()


void acceptClientsFork(int socketServerFd, struct sockaddr_in *socketAddress, socklen_t socketAddressLen);
void handleChildTerm(int signum);

#endif // ACCEPT_CLIENT_FORK_H
