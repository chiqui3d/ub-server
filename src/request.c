#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "request.h"
#include "../lib/color/color.h"
#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "helper.h"
#include "server.h"
#include "options.h"


bool isRequestComplete(char *buffer) {
    char *endOfRequest = strstr(buffer, "\r\n\r\n");
    if (endOfRequest != NULL) {
        return true;
    }
    return false;
}

void recvRequest(struct QueueConnectionElementType *connection) {

    while (1) {
        ssize_t bytesRead = recv(connection->clientFd, connection->requestBuffer + connection->requestBufferOffset, connection->requestBufferLength - connection->requestBufferOffset, 0);
        if (bytesRead < 0) {
            if (errno == EINTR) {
                // interrupted by a signal before any data was read
                continue;
            }
            // EWOULDBLOCK|EAGAIN, it not mean you're disconnected, it just means there's nothing to read now
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (isRequestComplete(connection->requestBuffer)) {
                    connection->requestBuffer[connection->requestBufferOffset] = 0;
                    connection->state = STATE_CONNECTION_SEND_HEADERS;
                    connection->requestBufferOffset = 0;
                    return;
                }
                return;
            }
            logError("recv() request failed. DoneForClose");
            connection->doneForClose = 1;
            return;
        }

        if (bytesRead == 0) {
            logDebug("0 bytes read, client disconnected");
            connection->doneForClose = 1;
            return;
        }

        connection->requestBufferOffset += bytesRead;
        // TODO: 413 Entity too large
    }
}

// false: bad request
bool processRequest(struct QueueConnectionElementType *connection) {

    char *buffer = connection->requestBuffer;
    strCopySafe(connection->scheme, "http");

    // get ip address from clientFd socket
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    if (getpeername(connection->clientFd, (struct sockaddr *)&clientAddr, &clientAddrLen) < 0) {
        logError("getpeername failed");
        return false;
    }

    char *ip = inet_ntoa(clientAddr.sin_addr);
    strCopySafe(connection->ip, ip);

    char *firstLine = strstr(buffer, "\r\n"); // CRLF
    if (NULL == firstLine) {
        logError("firstLine is NULL, no CRLF found. Report bad request");
        return false;
    }

    size_t firstLineLength = firstLine - buffer;
    char requestLine[firstLineLength + 1];

    strncpy(requestLine, buffer, firstLineLength);
    requestLine[firstLineLength] = '\0';
    //printf("requestLine:\n%s\n", requestLine);

    // buffer without first line and CRLF
    buffer += firstLineLength + 2;

    char method[10], path[REQUEST_PATH_MAX_SIZE], protocolVersion[9];
    sscanf(requestLine, "%s %s %s", method, path, protocolVersion);

    connection->path = strdup(path);
    connection->method = strToMethod(method);
    strCopySafe(connection->protocolVersion, protocolVersion);

    if (connection->method == METHOD_UNSUPPORTED) {
        logError("UNSUPPORTED method %s", method);
        return false;
    }

    char realPath[REQUEST_PATH_MAX_SIZE];
    strCopySafe(realPath, connection->path);
    char *tmpQuery = strchr(realPath, '?');
    if (tmpQuery != NULL) {
        *tmpQuery = '\0';
    }
    // is Index
    if (realPath[0] == '/' && realPath[1] == '\0') {
        strncat(realPath, "index.html", 11);
    }

    size_t absolutePathSize = strlen(OPTIONS.htmlDir) + strlen(realPath) + 1;
    char absolutePath[absolutePathSize];
    snprintf(absolutePath, absolutePathSize, "%s%s", OPTIONS.htmlDir, realPath);
    connection->absolutePath = strdup(absolutePath);

    //printf("buffer:\n%s\n", buffer);

    char *body = strstr(buffer, "\r\n\r\n"); // double CRLF pair to end of headers
    if (NULL == body) {
        logInfo("Request without headers");
        return true;
    }
    body += 4; // remove double CRLF pair to end of headers and beginning of body
    int bodyLength = strlen(body);
    body[bodyLength] = '\0';
    if (bodyLength > 0) {
        connection->requestBody = strdup(body);
    }

    //printf("body:\n%s\n", body);

    size_t headersSize = body - connection->requestBuffer - firstLineLength;
    char headersString[headersSize + 1];
    strncpy(headersString, connection->requestBuffer + firstLineLength, headersSize);
    headersString[headersSize] = '\0';
    //printf("headersString:\n%s\n", headersString);
   

    char *header = strtok(headersString, "\r\n");
    while (header != NULL) {

        size_t nameLen = strcspn(header, ": ");
        char *name = malloc((nameLen + 1) * sizeof(char));
        if (NULL == name) {
            die("malloc name");
        }
        memcpy(name, header, nameLen);
        name[nameLen] = '\0';

        size_t valueLen = strcspn(header + nameLen + 2, "\r\n");
        char *value = malloc((valueLen + 1) * sizeof(char));
        if (NULL == value) {
            die("malloc value");
        }
        memcpy(value, header + nameLen + 2, valueLen);
        value[valueLen] = '\0';

        struct Header *headerNode = malloc(sizeof(struct Header));
        if (NULL == headerNode) {
            die("malloc header");
        }

        char *nameLower = toLower(name, strlen(name));
        free(name);

        headerNode->name = nameLower;
        headerNode->value = value;
        headerNode->next = connection->requestHeaders;
        connection->requestHeaders = headerNode;
        header = strtok(NULL, "\r\n");
    }


    return true;
}

void printRequest(struct QueueConnectionElementType connection) {
    printf(RED "Request: \n" RESET);
    printf("Method: %s\n", methodToStr(connection.method));
    printf("Path: %s\n", connection.path);
    printf("Absolute path: %s\n", connection.absolutePath);
    printf("Protocol Version: %s\n", connection.protocolVersion);
    printf("IP: %s\n", connection.ip);
    printf("Scheme: %s\n", connection.scheme);
    printf("Headers:\n");
    struct Header *header = connection.requestHeaders;
    while (header != NULL) {
        printf("%s: %s\n", header->name, header->value);
        header = header->next;
    }
    if (connection.requestBody != NULL) {
        printf("Body: %s\n", connection.requestBody);
    }
    printf("\n\n");
}

void logRequest(struct QueueConnectionElementType connection) {

    size_t bodyLength = strlen(connection.requestBody == NULL ? "" : connection.requestBody);
    char *userAgent = getHeader(connection.requestHeaders, "user-agent");
    char *referer = getHeader(connection.requestHeaders, "referer");
    char *host = getHeader(connection.requestHeaders, "host");
    char URL[REQUEST_PATH_MAX_SIZE];

    if (host != NULL) {
        // TODO: not use host header for URL
        size_t URLLen = snprintf(NULL, 0, "%s%s%s%s",connection.scheme, "://", host, connection.path);
        snprintf(URL, URLLen + 1, "%s%s%s%s", connection.scheme, "://", host,connection.path);
    } else if (connection.path != NULL) {
        strCopySafe(URL, connection.path);
    } else {
        strncpy(URL, "<URL>", 6);
    }

    logInfo("Request | \"%s %s %s\" - %lu - %s - %s - %s - \"%s\"",
            methodToStr(connection.method),
            connection.path,
            connection.protocolVersion,
            bodyLength,
            connection.ip,
            URL,
            (referer != NULL ? referer : ""),
            (userAgent != NULL ? userAgent : ""));
}

const char *methodToStr(enum Method method) {
    return methodsList[method];
}

enum Method strToMethod(char *method) {
    int i;
    for (i = 0; i < METHOD_UNSUPPORTED; i++) {
        if (strcmp(method, methodsList[i]) == 0) {
            return (enum Method)i;
        }
    }

    return METHOD_UNSUPPORTED;
}
