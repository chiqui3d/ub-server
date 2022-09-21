#ifndef ACCEPT_CLIENT_THREAD_H
#define ACCEPT_CLIENT_THREAD_H


void acceptClientsThreadEpoll(int socketServerFd);
void *handleWithEpoll(void *threadDataArg);

#endif // ACCEPT_CLIENT_THREAD_H
