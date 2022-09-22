/**
 *
 * @brief Accept client connections with pthread only
 */

#include <arpa/inet.h> // for inet_ntoa()
#include <errno.h>     // for errno
#include <netdb.h>     // for getaddrinfo() and getservbyname
#include <pthread.h>   // for pthread_create()
#include <signal.h>    // for sigaction
#include <stdio.h>     // for fprintf()
#include <stdlib.h>    // for exit()
#include <string.h>    // for strlen()
#include <sys/wait.h>  // for waitpid
#include <unistd.h>    // for close()

#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "accept_client_thread.h"
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

void acceptClientsThread(int socketServerFd) {

    pthread_mutex_init(&threadDataMutex, 0); // remove, not used

    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        threads[i].socketFd = socketServerFd;
        pthread_create(&threads[i].thread, NULL, handleClient, (void *)&threads[i]);
        pthread_detach(threads[i].thread);
    }
    while (!sigintReceived){
        sleep(1);
    }

    for (i = 0; i < MAX_THREADS; i++) {
        pthread_cancel(threads[i].thread);
    }

    pthread_mutex_destroy(&threadDataMutex); // remove, not used
}

void *handleClient(void *threadDataArg) {

    struct threadData *threadData = (struct threadData *)threadDataArg;
    int socketServerFd = threadData->socketFd;

    while (true) {
        int clientFd;
        struct sockaddr_in client_address = {0};
        socklen_t client_address_len = sizeof(client_address);

        clientFd = accept(socketServerFd, (struct sockaddr *)&client_address, &client_address_len);

        if (clientFd < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            die("accept failed");
        }
        logDebug(RED "Connect with the client %d %s:%d and threadId %ld" RESET, clientFd, inet_ntoa(client_address.sin_addr), htons(client_address.sin_port), pthread_self());
        char buffer[BUFFER_REQUEST_SIZE];
        bool doneForClose = 0;
        int bytesRead = read(clientFd, buffer, BUFFER_REQUEST_SIZE - 1);
        if (bytesRead < 0) {
            die("read failed");
        }
        if (bytesRead == 0) {
            doneForClose = 1;
        }
        buffer[bytesRead] = '\0';

        if (doneForClose == 1) { // request close connection
            logDebug(BLUE "Done for close" RESET);
            close(clientFd);
            continue;
        }

        struct Request *request = makeRequest(buffer, clientFd);

        if (strncmp(request->protocolVersion, "HTTP/1.1", 8) != 0) {
            unsupportedProtocolResponse(clientFd, request->protocolVersion);
            logRequest(request);
            freeRequest(request);
            close(clientFd);
            continue;
        }
        if (strcmp(request->path, "/hello") == 0) {
            helloResponse(clientFd);
            logRequest(request);
            freeRequest(request);
            close(clientFd);
            continue;
        }

        // make response
        struct Response *response = makeResponse(request, OPTIONS.htmlDir);
        // send response
        sendResponse(response, request, clientFd);

        freeResponse(response);
        logRequest(request);
        freeRequest(request);
        close(clientFd);
    }
    // pthread_exit(NULL);

    return NULL;
}