#ifndef SERVER_H
#define SERVER_H

#define BUFFER_REQUEST_SIZE 1024
#define BUFFER_RESPONSE_SIZE 4096
#define MAX_QUEUE_CONNECTIONS 20

#include "options.h" // for struct Options
#include <signal.h>     // for sigaction

volatile sig_atomic_t sigintReceived;

int makeSocketNonBlocking(int sfd);

void serverRun(struct Options options);

#endif // SERVER_H