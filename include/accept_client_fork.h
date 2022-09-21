#ifndef ACCEPT_CLIENT_FORK_H
#define ACCEPT_CLIENT_FORK_H

#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket()


void acceptClientsFork(int socketServerFd);
void handleChildTerm(int signum);

#endif // ACCEPT_CLIENT_FORK_H
