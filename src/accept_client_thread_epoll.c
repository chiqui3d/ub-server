/**
 *
 * @brief Accept client connections with pthread and epoll
 *
 */

#include <arpa/inet.h> // for inet_ntoa()
#include <errno.h>     // for errno
#include <netdb.h>     // for getaddrinfo() and getservbyname
#include <pthread.h>   // for pthread_create()
#include <signal.h>    // for sigaction
#include <stdio.h>     // for fprintf()
#include <stdlib.h>    // for exit()
#include <string.h>    // for strlen()
#include <sys/epoll.h> // for epoll
#include <sys/wait.h>  // for waitpid
#include <unistd.h>    // for close()

#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "accept_client_epoll.h"
#include "accept_client_thread_epoll.h"

#include "helper.h"
#include "options.h"
#include "queue_connections.h"
#include "request.h"
#include "response.h"
#include "server.h"


struct Options OPTIONS;

struct threadData {
    pthread_t thread;
    int epollFd;
    int socketFd;
};

void acceptClientsThreadEpoll(int socketServerFd) {

    int epollFd = epoll_create1(0);
    if (epollFd < 0) {
        die("epoll_create failed");
    }
    addClient(epollFd, socketServerFd, 0);
    
    int nThreads = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    if (nThreads < 0) {
        die("sysconf nº processors failed");
    }
    struct threadData threads[nThreads];


    // create threads
    int i;
    for (i = 0; i < nThreads; i++) {
        threads[i].socketFd = socketServerFd;
        threads[i].epollFd = epollFd;
        pthread_create(&threads[i].thread, NULL, workThreadEpoll, (void *)&threads[i]);
        pthread_detach(threads[i].thread);
    }

    // we also use the main thread to manage the requests
    struct threadData *mainThreadData = (struct threadData *)malloc(sizeof(struct threadData));
    mainThreadData->epollFd = epollFd;
    mainThreadData->socketFd = socketServerFd;
    workThreadEpoll((void *)mainThreadData);

    // We wait until we obtain the SIGINT signal
    while (!sigintReceived) {
        sleep(1);
    }

    // We cancel all threads
    for (i = 0; i < nThreads; i++) {
        pthread_cancel(threads[i].thread);
    }
}

void *workThreadEpoll(void *threadDataArg) {

    struct threadData *threadData = (struct threadData *)threadDataArg;

    handleEpoll(threadData->socketFd, threadData->epollFd);

    return NULL;
}