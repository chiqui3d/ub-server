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
#include "queue_connections.h"
#include "server.h"

struct threadData {
    pthread_t thread;
    int socketFd;
};

void acceptClientsThreadEpoll(int socketServerFd) {

    int nThreads = sysconf(_SC_NPROCESSORS_ONLN);
    if (nThreads < 0) {
        die("sysconf nº processors failed");
    }
    struct threadData threads[nThreads];

    // create threads
    int i;
    for (i = 0; i < nThreads; i++) {
        threads[i].socketFd = socketServerFd;
        pthread_create(&threads[i].thread, NULL, workThreadEpoll, (void *)&threads[i]);
        pthread_detach(threads[i].thread);
    }

    // We wait until we obtain the SIGINT signal
    while (!sigintReceived) {
        sleep(1);
    }
    // send a signal to exit the epoll loop and free any memory allocated
    for (i = 0; i < nThreads; i++) {
        pthread_kill(threads[i].thread, SIGINT);
        // pthread_cancel(threads[i].thread);
    }
    usleep(100000); // time for threads to finish
}

void *workThreadEpoll(void *threadDataArg) {
    struct threadData *threadData = (struct threadData *)threadDataArg;

    handleEpollFacade(threadData->socketFd);

    return NULL;
}