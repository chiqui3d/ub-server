#include <arpa/inet.h> // for inet_ntoa()
#include <errno.h>     // for errno
#include <netdb.h>     // for getaddrinfo() and getservbyname
#include <stdio.h>     // for fprintf()
#include <stdlib.h>    // for exit()
#include <string.h>    // for strlen()
#include <unistd.h>    // for close()

#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "options.h"
#include "request.h"
#include "response.h"
#include "server.h"
#include "server_accept_epoll.h"

struct Options OPTIONS;

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
    struct epoll_event event = buildEvent(EPOLLIN | EPOLLET, clientFd);
    int s = epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event);
    if (s == -1) {
        die("EPOLL_CTL_ADD failed");
    }
}

/* closeClient(int clientFd, struct epoll_event *events, int *eventsCount) {
    close(clientFd);
    for (int i = 0; i < *eventsCount; i++) {
        if (events[i].data.fd == clientFd) {
            events[i].data.fd = -1;
            break;
        }
    }
} */

void waitAndAccept(int socketServerFd, struct sockaddr_in *socketAddress, socklen_t socketAddressLen) {

    int epollFd = epoll_create1(0);
    if (epollFd < 0) {
        die("epoll_create failed");
    }
    addClient(epollFd, socketServerFd);

    struct epoll_event events[MAX_EPOLL_EVENTS];

    while (!sigintReceived) {
        int i, num_ready;
        num_ready = epoll_wait(epollFd, events, MAX_EPOLL_EVENTS, -1 /*timeout*/);
        if (num_ready < 0) {
            logWarning("epoll_wait failed");
        }
        for (i = 0; i < num_ready; i++) {
            if (events[i].data.fd == socketServerFd) {
                consoleDebug(GREEN "Accepting new connection........." RESET);
                acceptNewConnections(epollFd, socketServerFd);
            } else if (events[i].events & EPOLLIN) {
                int clientFd = events[i].data.fd;
                consoleDebug(GREEN "Reading from client fd %i........." RESET, clientFd);
                char buffer[BUFFER_REQUEST_SIZE] = "0";
                readRequest(buffer, epollFd, &clientFd);
               
                if (clientFd != -1 && strlen(buffer) > 0) {
                    // consoleDebug("Buffer:\n%s", buffer);
                    struct Request *request = makeRequest(buffer, clientFd);
                    if (strcmp(request->path, "/hello") == 0) {
                        helloResponse(clientFd);
                        freeRequest(request);
                        continue;
                    }
                    struct Response *response = makeResponse(request, OPTIONS.htmlDir);

                    char *connectionHeader = getHeader(request->headers, "connection");
                    bool closeConnection = false;
                    // if not keep-alive, close connection
                    if (connectionHeader == NULL || *connectionHeader != 'k') {
                        closeConnection = true;
                        response->headers = addHeader(response->headers, "connection", "close");
                    }
                    sendResponse(response, clientFd);
                    if (closeConnection) {
                        closeClient(epollFd, clientFd);
                    }
                    // printRequest(request);
                    freeResponse(response);
                    logRequest(request);
                    freeRequest(request);
                    continue;
                } else {
                    if (clientFd != -1) {
                        closeClient(epollFd, clientFd);
                    }
                }

            } else if ((events[i].events & EPOLLERR) ||
                       (events[i].events & EPOLLHUP) ||
                       (events[i].events & EPOLLRDHUP) ||
                       (!(events[i].events & EPOLLIN))) {
                // TODO: never happens, remove?
                logDebug("EPOLLERR|EPOLLHUP|EPOLLRDHUP");
            }
        }
    }
}

void acceptNewConnections(int epollFd, int socketServerFd) {
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
            errno = 0;
            break;
        }
        if (clientFd >= MAX_CONNECTIONS) {
            close(clientFd);
            logWarning("socket fd (%d) >= MAX_CONNECTIONS (%d)", clientFd, MAX_CONNECTIONS);
            return;
        }

        logDebug("Connect with the client %d %s:%d", clientFd, inet_ntoa(client_address.sin_addr), htons(client_address.sin_port));
        makeSocketNonBlocking(clientFd);
        addClient(epollFd, clientFd);
    }
}

char *readRequest(char *buffer, int epollFd, int *clientFd) {
    /* We have data on the fd waiting to be read. Read and
        display it. We must read whatever data is available
        completely, as we are running in edge-triggered mode
        and won't get a notification again for the same
        data. */

    int doneForClose = 0;
    int totalBytesRead = 0;
    int restBytesRead = BUFFER_REQUEST_SIZE - 1; // for add null terminator char
    while (restBytesRead > 0) {                  // MSG_PEEK
        ssize_t bytesRead = recv(*clientFd, buffer + totalBytesRead, restBytesRead, 0);
        if (bytesRead < 0) {
            // EAGAIN does not mean you're disconnected,
            // it just means "there's nothing to read now; try again later"
            if (errno != EAGAIN || errno != EWOULDBLOCK) {
                // possible ECONNRESET or EPIPE with wrk program
                logError("recv() request failed");
                doneForClose = 1;
            }
            errno = 0;
            break;
        } else if (bytesRead == 0) {
            doneForClose = 1;
            break;
        } else {
            totalBytesRead += bytesRead;
            restBytesRead -= bytesRead;
        }
    }
    buffer[totalBytesRead] = '\0';

    if (doneForClose) {
        closeClient(epollFd, *clientFd);
        *clientFd = -1;
    }

    return buffer;
}