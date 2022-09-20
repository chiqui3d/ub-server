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
#include "queue_connections.h"
#include "helper.h"
#include "options.h"
#include "request.h"
#include "response.h"
#include "server.h"

struct Options OPTIONS;
struct QueueConnectionsType QueueConnections;
int IndexQueueConnectionsFd[MAX_CONNECTIONS];

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
        // calculate epoll timeout
        time_t now = time(NULL);
        struct QueueConnectionElementType lastConnectionQueueElement = peekQueueConnections();
        int timeout = -1;
        if (lastConnectionQueueElement.fd != 0) {
            timeout = (now - lastConnectionQueueElement.priorityTime) * 1000;
        }

        logDebug(BLUE "timeout: %d" RESET, timeout);

        // check for CLOSE client connection by time_t
        while (lastConnectionQueueElement.fd != 0 && difftime(now, lastConnectionQueueElement.priorityTime) >= KEEP_ALIVE_TIMEOUT) {
            char date[DATETIME_HELPER_SIZE];
            timeToDatetimeString(lastConnectionQueueElement.priorityTime, date);
            logDebug(BLUE "Close by timeout with fd %i and dateTime %s" RESET, lastConnectionQueueElement.fd, date);
            dequeueConnection();
            closeClient(epollFd, lastConnectionQueueElement.fd);
            lastConnectionQueueElement = peekQueueConnections();
        }

        int i, num_ready;
        // -1 block forever, 0 non-blocking, > 0 timeout in milliseconds
        num_ready = epoll_wait(epollFd, events, MAX_EPOLL_EVENTS, timeout);
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
                if (doneForClose == 1) { // request close connection
                    logDebug(BLUE "Done for close" RESET);
                    dequeueConnectionByFd(clientFd);
                    closeClient(epollFd, clientFd);
                    continue;
                }

                // consoleDebug("Buffer:\n%s", buffer);
                struct Request *request = makeRequest(buffer, clientFd);

                // Hardcoded for now
                if (strncmp(request->protocolVersion, "HTTP/1.1", 8) != 0) {
                    unsupportedProtocolResponse(clientFd, request->protocolVersion);
                    logRequest(request);
                    freeRequest(request);
                    continue;
                }
                if (strcmp(request->path, "/hello") == 0) {
                    helloResponse(clientFd);
                    logRequest(request);
                    freeRequest(request);
                    continue;
                }


                // make response
                struct Response *response = makeResponse(request, OPTIONS.htmlDir);               
                // send response
                sendResponse(response, request, clientFd);

                if (response->closeConnection == false) {
                    if (IndexQueueConnectionsFd[clientFd] == -1) {
                        struct QueueConnectionElementType connectionQueueElement = {time(NULL), clientFd};
                        enqueueConnection(connectionQueueElement);
                    } else {
                        // update priorityTime and re-order queue
                        updateQueueConnection(clientFd, time(NULL));
                    }
                } else {
                    dequeueConnectionByFd(clientFd);
                    closeClient(epollFd, clientFd);
                }
                // clean and log
                // printRequest(request);
                freeResponse(response);
                logRequest(request);
                freeRequest(request);

            } else if ((events[i].events & EPOLLERR) ||
                       (events[i].events & EPOLLHUP) ||
                       (events[i].events & EPOLLRDHUP)) {
                // TODO: never happens, remove?
                logDebug("EPOLLERR|EPOLLHUP|EPOLLRDHUP");
            }
        }

        // printQueueConnections();
    }
}

void acceptConnection(int epollFd, int socketServerFd) {
    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int clientFd = accept(socketServerFd, (struct sockaddr *)&client_address, &client_address_len);
        // alternative to makeSocketNonBlocking function with fcntl
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