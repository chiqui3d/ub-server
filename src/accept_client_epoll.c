#include <arpa/inet.h> // for inet_ntoa()
#include <errno.h>     // for errno
#include <netdb.h>     // for getaddrinfo() and getservbyname
#include <stdio.h>     // for fprintf()
#include <stdlib.h>    // for exit()
#include <string.h>    // for strlen()
#include <unistd.h>    // for close()

#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "handle_timeout_connections.h"
#include "helper.h"
#include "options.h"
#include "request.h"
#include "response.h"
#include "server.h"
#include "accept_client_epoll.h"


struct Options OPTIONS;
struct QueueConnectionsTimeout queueConnectionsTimeout;
int indexQueueConnectionsFd[MAX_CONNECTIONS];

struct epoll_event buildEvent(int events, int fd) {
    struct epoll_event event = {0};
    event.events = events;
    event.data.fd = fd;
    return event;
}

void closeClient(int epollFd, int clientFd) {
    // logDebug("Delete file descriptor %d from epoll", clientFd);
    int s = epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    if (s == -1 && errno != EBADF) {
        die("EPOLL_CTL_DEL failed");
    }
    logDebug("Closed connection on descriptor %d", clientFd);
    close(clientFd);
}

void addClient(int epollFd, int clientFd) {
    // (EPOLLIN | EPOLLET Edge Triggered (ET) for non-blocking sockets
    // EPOLLERR and EPOLLHUP are always included even if you're not requesting them
    struct epoll_event event = buildEvent(EPOLLIN | EPOLLET | EPOLLHUP | EPOLLRDHUP, clientFd);
    int s = epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event);
    if (s == -1) {
        die("EPOLL_CTL_ADD failed");
    }
}

void acceptClients(int socketServerFd, struct sockaddr_in *socketAddress, socklen_t socketAddressLen) {

    int epollFd = epoll_create1(0);
    if (epollFd < 0) {
        die("epoll_create failed");
    }
    addClient(epollFd, socketServerFd);

    struct epoll_event events[MAX_EPOLL_EVENTS];

    createQueueConnections();

    while (!sigintReceived) {
        int i, num_ready;
        // -1 block forever, 0 non-blocking, > 0 timeout in milliseconds
        // -1 is necessary for accept new connections
        num_ready = epoll_wait(epollFd, events, MAX_EPOLL_EVENTS, (int)(KEEP_ALIVE_TIMEOUT / 2) * 1000 /*timeout*/);
        if (num_ready < 0) {
            logWarning("epoll_wait failed");
        }
        for (i = 0; i < num_ready; i++) {
            if (events[i].data.fd == socketServerFd) {
                consoleDebug(GREEN "Accepting new connection........." RESET);
                acceptConnection(epollFd, socketServerFd);
            } else if (events[i].events & EPOLLIN) {
                int clientFd = events[i].data.fd;
                consoleDebug(GREEN "Reading from client fd %i........." RESET, clientFd);
                char buffer[BUFFER_REQUEST_SIZE];
                bool doneForClose = 0;
                readRequest(buffer, clientFd, &doneForClose);
                if (doneForClose == 1) {
                    logDebug(BLUE "Done for close" RESET);
                    dequeueConnectionByFd(clientFd);
                    closeClient(epollFd, clientFd);
                    clientFd = -1;
                    continue;
                }

                if (clientFd != -1 && strlen(buffer) > 0) {
                    // consoleDebug("Buffer:\n%s", buffer);
                    struct Request *request = makeRequest(buffer, clientFd);
                    // Hardcoded for now
                    if (strncmp(request->protocolVersion, "HTTP/1.1", 8) != 0) {
                        unsupportedProtocolResponse(clientFd, request->protocolVersion);
                        freeRequest(request);
                        continue;
                    }
                    if (strcmp(request->path, "/hello") == 0) {
                        helloResponse(clientFd);
                        freeRequest(request);
                        continue;
                    }
                    // make response
                    struct Response *response = makeResponse(request, OPTIONS.htmlDir);

                    // connection
                    char *connectionHeader = getHeader(request->headers, "connection");
                    bool closeConnection = false;

                    // if not keep-alive in the request header, close connection
                    if (connectionHeader == NULL || (connectionHeader != NULL && *connectionHeader == 'c')) {
                        closeConnection = true;
                        response->headers = addHeader(response->headers, "connection", "close");
                    }
                    if (connectionHeader != NULL && *connectionHeader == 'k') {
                        response->headers = addHeader(response->headers, "connection", "keep-alive");
                        char headerKeepAliveValue[11];
                        sprintf(headerKeepAliveValue, "timeout=%i", KEEP_ALIVE_TIMEOUT);
                        response->headers = addHeader(response->headers, "keep-alive", headerKeepAliveValue);
                    }

                    // response
                    sendResponse(response, clientFd);
                    // printRequest(request);
                    freeResponse(response);
                    logRequest(request);
                    freeRequest(request);
                    if (closeConnection == false) {
                        if (indexQueueConnectionsFd[clientFd] == -1) {
                            struct ConnectionTimeoutElement connectionQueueElement = {time(NULL), clientFd};
                            enqueueConnection(connectionQueueElement);
                        } else {
                            updateQueueConnection(clientFd, time(NULL));
                        }
                    } else {
                        dequeueConnectionByFd(clientFd);
                        closeClient(epollFd, clientFd);
                    }
                }

            } else if ((events[i].events & EPOLLERR) ||
                       (events[i].events & EPOLLHUP) ||
                       (events[i].events & EPOLLRDHUP)) {
                // TODO: never happens, remove?
                logDebug("EPOLLERR|EPOLLHUP|EPOLLRDHUP");
            }
        }

        // check connection client timeout

        struct ConnectionTimeoutElement lastConnectionQueueElement = peekQueueConnections();
        while (lastConnectionQueueElement.fd != 0 && difftime(time(NULL), lastConnectionQueueElement.priorityTime) >= KEEP_ALIVE_TIMEOUT) {
            logDebug(BLUE "Close by timeout with fd %i and time_t %ld" RESET, lastConnectionQueueElement.fd, lastConnectionQueueElement.priorityTime);
            dequeueConnection();
            closeClient(epollFd, lastConnectionQueueElement.fd);
            lastConnectionQueueElement = peekQueueConnections();
        }
        char datetime[DATETIME_HELPER_SIZE];
        timeToDatetimeString(time(NULL), datetime);
        logDebug("FDs processed, now waiting again for %d seconds in the time: %s", (int)(KEEP_ALIVE_TIMEOUT / 2), datetime);
        printQueueConnections();
    }
}

void acceptConnection(int epollFd, int socketServerFd) {
    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int clientFd = accept(socketServerFd, (struct sockaddr *)&client_address, &client_address_len);
        // alternative to makeSocketNonBlocking
        // int client_fd = accept4(socket_server_fd, (struct sockaddr *)&client_address, (socklen_t *)&client_address_len, SOCK_NONBLOCK);

        if (clientFd < 0) {
            if (errno != EAGAIN || errno != EWOULDBLOCK) {
                die("accept() failed");
            }
            break;
        }
        if (clientFd >= MAX_CONNECTIONS) {
            logWarning("Maximum connections (%d) exceeded by fd %d", MAX_CONNECTIONS, clientFd);
            tooManyRequestResponse(clientFd);
            close(clientFd);
            return;
        }

        logDebug("Connect with the client %d %s:%d", clientFd, inet_ntoa(client_address.sin_addr), htons(client_address.sin_port));
        makeSocketNonBlocking(clientFd);
        addClient(epollFd, clientFd);
    }
}