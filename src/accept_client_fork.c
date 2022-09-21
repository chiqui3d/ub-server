/**
 *
 * @brief Accept client connections with fork()
 *
 * This is just a Fork test to compare it with Epoll, according to the tests,
 * it seems that Epoll is winning, for fork, it would be necessary to create the prefork system,
 * to get similar or better. To test it, you would have make in the server.c file, the:
 *
 * * Comment the `makeSocketNonBlocking` function
 * * Uncomment the `#include "accept_client_fork.h"` header file
 * * Uncomment the `acceptClientsFork` function
 * * Comment the `acceptClientsEpoll` function
 */

#include <arpa/inet.h> // for inet_ntoa()
#include <errno.h>     // for errno
#include <netdb.h>     // for getaddrinfo() and getservbyname
#include <signal.h>    // for sigaction
#include <stdio.h>     // for fprintf()
#include <stdlib.h>    // for exit()
#include <string.h>    // for strlen()
#include <sys/wait.h>  // for waitpid
#include <unistd.h>    // for close()

#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "accept_client_fork.h"
#include "helper.h"
#include "options.h"
#include "request.h"
#include "response.h"
#include "server.h"

struct Options OPTIONS;

void handleChildTerm(int signum) {
    // avoid zombie processes
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = 0; // for avoid return: no child processes
}

void acceptClientsFork(int socketServerFd) {

    struct sigaction sa; // signal handler
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = &handleChildTerm;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }
    while (!sigintReceived) {
        int clientFd;
        pid_t clientPid;
        struct sockaddr_in client_address = {0};
        socklen_t client_address_len = sizeof(client_address);

        /**
         * @brief https://man7.org/linux/man-pages/man7/socket.7.html
         * An alternative to poll(2) and select(2) is to let the kernel
         * inform the application about events via a SIGIO signal.  For that
         * the O_ASYNC flag must be set on a socket file descriptor via
         * fcntl(2) and a valid signal handler for SIGIO must be installed
         * via sigaction(2).  See the Signals discussion below.
         * 
         */
        clientFd = accept(socketServerFd, (struct sockaddr *)&client_address, &client_address_len);

        if (clientFd < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            die("accept failed");
        }
        logDebug("Connect with the client %d %s:%d", clientFd, inet_ntoa(client_address.sin_addr), htons(client_address.sin_port));

        clientPid = fork();
        if (clientPid < 0) { // error
            die("fork failed");
        }
        if (clientPid == 0) { /* child process */
            close(socketServerFd);

            consoleDebug(GREEN "Reading from client fd %i........." RESET, clientFd);

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
                exit(EXIT_SUCCESS);
            }

            struct Request *request = makeRequest(buffer, clientFd);

            if (strncmp(request->protocolVersion, "HTTP/1.1", 8) != 0) {
                unsupportedProtocolResponse(clientFd, request->protocolVersion);
                logRequest(request);
                freeRequest(request);
                close(clientFd);
                exit(EXIT_SUCCESS);
            }
            if (strcmp(request->path, "/hello") == 0) {
                helloResponse(clientFd);
                logRequest(request);
                freeRequest(request);
                close(clientFd);
                exit(EXIT_SUCCESS);
            }

            // make response
            struct Response *response = makeResponse(request, OPTIONS.htmlDir);
            // send response
            sendResponse(response, request, clientFd);

            freeResponse(response);
            logRequest(request);
            freeRequest(request);
            close(clientFd);
            // kill(getpid(), SIGKILL);
            exit(EXIT_SUCCESS);
        }
        close(clientFd);
    }
}
