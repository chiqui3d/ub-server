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
#include "accept_client_thread_epoll.h"
#include "accept_client_epoll.h"

#include "helper.h"
#include "options.h"
#include "queue_connections.h"
#include "request.h"
#include "response.h"
#include "server.h"

#define MAX_THREADS 5

struct Options OPTIONS;

pthread_mutex_t threadDataMutex;
struct threadData {
    pthread_t thread;
    int socketFd;
};
struct threadData threads[MAX_THREADS];

void acceptClientsThreadEpoll(int socketServerFd) {

    pthread_mutex_init(&threadDataMutex, 0); // remove, not used

    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        threads[i].socketFd = socketServerFd;
        pthread_create(&threads[i].thread, NULL, handleWithEpoll, (void *)&threads[i]);
        pthread_detach(threads[i].thread);
    }
    while (!sigintReceived) {
        sleep(1);
    }

    for (i = 0; i < MAX_THREADS; i++) {
        pthread_cancel(threads[i].thread);
    }

    pthread_mutex_destroy(&threadDataMutex); // remove, not used
}

void *handleWithEpoll(void *threadDataArg) {
    struct threadData *threadData = (struct threadData *)threadDataArg;

    int epollFd = epoll_create1(0);
    if (epollFd < 0) {
        die("epoll_create failed");
    }
    addClient(epollFd, threadData->socketFd);

    struct epoll_event events[1000];
    while (!sigintReceived) {
        int i, num_ready;
        // -1 block forever, 0 non-blocking, > 0 timeout in milliseconds
        num_ready = epoll_wait(epollFd, events, 1000, -1);
        if (num_ready < 0) {
            logWarning("epoll_wait failed");
        }
        for (i = 0; i < num_ready; i++) {
            if (events[i].data.fd == threadData->socketFd) {
                // accept new client
                consoleDebug("Accepting new connection.........");
                acceptConnection(epollFd, threadData->socketFd);

            } else if (events[i].events & EPOLLIN) {
                // read from client
                int clientFd = events[i].data.fd;

                consoleDebug("Reading from client fd %i.........", clientFd);

                char buffer[BUFFER_REQUEST_SIZE];
                bool doneForClose = 0;
                readRequest(buffer, clientFd, &doneForClose);

                if (doneForClose == 1) { // request close connection
                    logDebug(BLUE "Done for close" RESET);
                    dequeueConnectionByFd(clientFd);
                    closeClient(epollFd, clientFd);
                    continue;
                }

                struct Request *request = makeRequest(buffer, clientFd);

                // TODO: Refactoring. Hardcoded for now
                if (strncmp(request->protocolVersion, "HTTP/1.1", 8) != 0) {
                    unsupportedProtocolResponse(clientFd, request->protocolVersion);
                    logRequest(request);
                    freeRequest(request);
                    closeClient(epollFd, clientFd);
                    continue;
                }
                // TODO: Router system
                if (strcmp(request->path, "/hello") == 0) {
                    helloResponse(clientFd);
                    logRequest(request);
                    freeRequest(request);
                    closeClient(epollFd, clientFd);
                    continue;
                }

                // make response
                struct Response *response = makeResponse(request, OPTIONS.htmlDir);
                // send response
                sendResponse(response, request, clientFd);

                if (response->closeConnection == true) {
                    closeClient(epollFd, clientFd);
                }
                // free and log
                freeResponse(response);
                logRequest(request);
                freeRequest(request);

            }
        }
    }

    return NULL;
}