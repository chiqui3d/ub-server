#include <arpa/inet.h> // for inet_ntoa()
#include <errno.h>     // for errno
#include <netdb.h>     // for getaddrinfo() and getservbyname
#include <stdio.h>     // for fprintf()
#include <stdlib.h>    // for exit()
#include <string.h>    // for strlen()
#include <unistd.h>    // for close()

#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "accept_client_epoll.h"
#include "helper.h"
#include "options.h"
#include "queue_connections.h"
#include "request.h"
#include "response.h"
#include "server.h"

static pthread_mutex_t threadDataMutex = PTHREAD_MUTEX_INITIALIZER;

void handleEpollFacade(int socketServerFd) {

    int epollFd = epoll_create1(0);
    if (epollFd < 0) {
        die("epoll_create failed");
    }
    addEpollClient(epollFd, socketServerFd, 0);

    struct epoll_event events[MAX_EPOLL_EVENTS];
    struct QueueConnectionsType queueConnections = createQueueConnections();

    handleEpoll(socketServerFd, epollFd, events, &queueConnections);
}

void handleEpoll(int socketServerFd, int epollFd, struct epoll_event *events, struct QueueConnectionsType *queueConnections) {

    long int threadId = pthread_self();

    while (!sigintReceived) {
        // calculate epoll timeout
        time_t now = time(NULL);
        struct QueueConnectionElementType firstConnectionQueueElement = peekQueueConnections(queueConnections);
        int timeout = -1;
        if (firstConnectionQueueElement.clientFd != 0) {
            timeout = (now - firstConnectionQueueElement.priorityTime) * 1000;
        }
        // check for CLOSE client connection by time_t
        while (firstConnectionQueueElement.clientFd != 0 && difftime(now, firstConnectionQueueElement.priorityTime) >= KEEP_ALIVE_TIMEOUT) {
            char date[DATETIME_HELPER_SIZE];
            timeToDatetimeString(firstConnectionQueueElement.priorityTime, date);
            logDebug("Close by timeout with fd %i and dateTime %s", firstConnectionQueueElement.clientFd, date);
            dequeueConnection(queueConnections);
            closeEpollClient(epollFd, firstConnectionQueueElement.clientFd);
            firstConnectionQueueElement = peekQueueConnections(queueConnections);
        }

        int i, readyEventClients;
        // -1 block forever, 0 non-blocking, > 0 timeout in milliseconds
        readyEventClients = epoll_wait(epollFd, events, MAX_EPOLL_EVENTS, timeout);
        if (readyEventClients < 0) {
            if (errno == EINTR) {
                // avoid error when we receive a signal
                continue;
            }
            logWarning("epoll_wait failed");
        }
        for (i = 0; i < readyEventClients; i++) {
            if (events[i].data.fd == socketServerFd) {
                logDebug("Accepting new connection in the thread %ld", threadId);
                //  pthread_mutex_lock(&threadDataMutex);
                acceptEpollConnection(epollFd, socketServerFd, EPOLLIN | EPOLLET);
                // pthread_mutex_unlock(&threadDataMutex);
            } else if (events[i].events & EPOLLIN) {

                int clientFd = events[i].data.fd;
                struct QueueConnectionElementType *connection = getConnectionByFd(queueConnections, clientFd);

                // simple state machine
                bool repeat = true;
                do {
                    switch (connection->state) {
                        case STATE_CONNECTION_RECV: {
                            logDebug("STATE_CONNECTION_RECV with fd %i", clientFd);
                            recvRequest(connection);
                            break;
                        }
                        case STATE_CONNECTION_SEND_HEADERS: {
                            logDebug("STATE_CONNECTION_SEND_HEADERS with fd %i", clientFd);
                            if (connection->requestHeaders == NULL) {
                                logDebug("processRequest with fd %i", clientFd);
                                bool isValidRequest = processRequest(connection);


                                if (!isValidRequest) {
                                    logDebug("badRequestResponse with fd %i", clientFd);
                                    badRequestResponse(clientFd);
                                    logRequest(*connection);
                                    connection->state = STATE_CONNECTION_DONE_FOR_CLOSE;
                                    break;
                                }
                                // check supported protocol
                                char *protocol = connection->protocolVersion;
                                if (strcmp(protocol, "HTTP/1.0") != 0 && strcmp(protocol, "HTTP/1.1") != 0) {
                                    unsupportedProtocolResponse(clientFd, protocol);
                                    logRequest(*connection);
                                    connection->state = STATE_CONNECTION_DONE_FOR_CLOSE;
                                    break;
                                }

                                // hello response
                                if (strcmp(connection->path, "/hello") == 0) {
                                    helloResponse(clientFd);
                                    logRequest(*connection);
                                    connection->state = STATE_CONNECTION_DONE_FOR_CLOSE;
                                    break;
                                }
                            }
                            if (connection->responseBufferHeaders == NULL) {
                                makeResponse(connection);
                            }

                            sendResponseHeaders(connection);
                            break;
                        }
                        case STATE_CONNECTION_SEND_BODY: {
                            logDebug("STATE_CONNECTION_SEND_BODY with fd %i", clientFd);
                            sendResponseFile(connection);
                            break;
                        }
                        case STATE_CONNECTION_DONE: {
                            logDebug("STATE_CONNECTION_DONE with fd %i", clientFd);
                            logRequest(*connection);
                            if (connection->keepAlive == true) {
                                // rearm the file descriptor with a new event mask
                                // modClient(epollFd, clientFd, EPOLLIN | EPOLLET | EPOLLONESHOT);
                                updateQueueConnection(queueConnections, clientFd);
                                repeat = false;
                            } else {
                                connection->state = STATE_CONNECTION_DONE_FOR_CLOSE;
                            }
                            break;
                        }
                        case STATE_CONNECTION_DONE_FOR_CLOSE: {
                            logDebug("STATE_CONNECTION_DONE_FOR_CLOSE with fd %i", clientFd);
                            dequeueConnectionByFd(queueConnections, clientFd);
                            closeEpollClient(epollFd, clientFd);
                            repeat = false;
                            break;
                        }
                       
                    }
                } while (repeat);
            }
        }

        // printQueueConnections(queueConnections);
    }

    logDebug("sigIntReceived in the thread %ld", threadId);
    unsigned short int i;
    for (i = 0; i < queueConnections->currentSize; i++) {
        freeConnection(&queueConnections->connections[i]);
    }
}

// Facade for test one epoll per thread

void acceptEpollConnection(int epollFd, int socketServerFd, int events) {
    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int clientFd = accept(socketServerFd, (struct sockaddr *)&client_address, &client_address_len);
        // alternative to makeSocketNonBlocking function with fcntl, we save a call to the system
        // int client_fd = accept4(socket_server_fd, (struct sockaddr *)&client_address, (socklen_t *)&client_address_len, SOCK_NONBLOCK);
        if (clientFd < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                logWarning("accept() failed");
            }
            break;
        }

        logDebug("Connect with the client %d %s:%d", clientFd, inet_ntoa(client_address.sin_addr), htons(client_address.sin_port));
        makeSocketNonBlocking(clientFd);
        addEpollClient(epollFd, clientFd, events);
    }
}

struct epoll_event buildEpollEvent(int events, int fd) {
    struct epoll_event event = {0};
    event.events = events;
    event.data.fd = fd;
    return event;
}

void closeEpollClient(int epollFd, int clientFd) {
    // logDebug("Delete file descriptor %d from epoll", clientFd);
    int s = epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    if (s == -1 && errno != EBADF) {
        die("EPOLL_CTL_DEL failed");
    }
    logDebug("Closed connection on descriptor %d", clientFd);
    close(clientFd);
}

void addEpollClient(int epollFd, int clientFd, int events) {
    if (events == 0) {
        events = EPOLLIN | EPOLLET;
    }
    // (EPOLLIN | EPOLLET Edge Triggered (ET) for non-blocking sockets
    // EPOLLERR and EPOLLHUP are always included even if you're not requesting them
    struct epoll_event event = buildEpollEvent(events, clientFd);
    int s = epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event);
    if (s == -1) {
        die("EPOLL_CTL_ADD failed");
    }
}

void modEpollClient(int epollFd, int clientFd, int events) {
    if (events == 0) {
        events = EPOLLIN | EPOLLET;
    }
    struct epoll_event event = buildEpollEvent(events, clientFd);
    int s = epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &event);
    if (s == -1) {
        die("EPOLL_CTL_MOD failed");
    }
}