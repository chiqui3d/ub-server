#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../lib/color/color.h"
#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "request.h"
#include "helper.h"
#include "server.h"


char *readRequest(char *buffer, int clientFd, bool *doneForClose) {

    int totalBytesRead = 0;
    int restBytesRead = BUFFER_REQUEST_SIZE - 1; // for add null terminator char
    while (restBytesRead > 0) {                  // MSG_PEEK
        ssize_t bytesRead = recv(clientFd, buffer + totalBytesRead, restBytesRead, 0);
        if (bytesRead < 0) {
            // Ignore EWOULDBLOCK|EAGAIN, it not mean you're disconnected, it just means there's nothing to read now 
            if (errno != EAGAIN || errno != EWOULDBLOCK) {
                // possible ECONNRESET or EPIPE with wrk program
                logError("recv() request failed");
                *doneForClose = 1;
            }
            break;
        } else if (bytesRead == 0) {
            *doneForClose = 1;
            break;
        } else {
            totalBytesRead += bytesRead;
            restBytesRead -= bytesRead;
        }
    }
    buffer[totalBytesRead] = '\0';


    return buffer;
}

struct Request *makeRequest(char *buffer, int clientFd) {

    struct Request *request = malloc(sizeof(struct Request));
    if (NULL == request) {
        die("malloc request");
    }

    memset(request, 0, sizeof(struct Request));

    strcpy(request->scheme, "http");

    // get ip address from clientFd socket
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    if (getpeername(clientFd, (struct sockaddr *)&clientAddr, &clientAddrLen) < 0) {
        die("getpeername");
    }

    char *ip = inet_ntoa(clientAddr.sin_addr);
    strcpy(request->ip, ip);

    char *firstLine = strstr(buffer, "\r\n"); // CRLF
    if (NULL == firstLine) {
        die("strstr firstLine");
    }

    size_t firstLineLength = firstLine - buffer;
    char requestLine[firstLineLength + 1];

    strncpy(requestLine, buffer, firstLineLength);
    requestLine[firstLineLength] = '\0';

    // buffer without first line and CRLF
    buffer += firstLineLength + 2;

    char method[10], path[100], protocolVersion[9];

    sscanf(requestLine, "%s %s %s", method, path, protocolVersion);

    request->method = strToMethod(method);

    if (request->method == METHOD_UNSUPPORTED) {
        die("UNSUPPORTED method %s", method);
    }

    request->path = strdup(path);
    request->protocolVersion = strdup(protocolVersion);
   

    char *body = strstr(buffer, "\r\n\r\n"); // double CRLF pair to end of headers

    if (NULL == body) {
        die("strstr body, not double CRLF pair found");
    }
    body += 4; // remove double CRLF pair to end of headers and beginning of body
    int bodyLength = strlen(body);
    body[bodyLength] = '\0';
    if (bodyLength > 0) {
        request->body = strdup(body);
    }

    char *headersString = malloc((body - buffer + 1) * sizeof(char));
    if (NULL == headersString) {
        die("malloc headers");
    }
    memcpy(headersString, buffer, body - buffer);
    headersString[body - buffer] = '\0';

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
        headerNode->next = request->headers;
        request->headers = headerNode;
        header = strtok(NULL, "\r\n");
    }

    if (headersString != NULL) {
        free(headersString);
    }

    return request;
}

void freeRequest(struct Request *request) {
    
    free(request->path);
    free(request->protocolVersion);
    if (request->body != NULL) {
        free(request->body);
    }
    
    freeHeader(request->headers);
    free(request);
}

void printRequest(struct Request *request) {
    printf(RED "Request: \n" RESET);
    printf("Method: %s\n", methodToStr(request->method));
    printf("Path: %s\n", request->path);
    printf("Protocol Version: %s\n", request->protocolVersion);
    printf("IP: %s\n", request->ip);
    printf("Scheme: %s\n", request->scheme);
    printf("Headers:\n");
    struct Header *header = request->headers;
    while (header != NULL) {
        printf("%s: %s\n", header->name, header->value);
        header = header->next;
    }
    if (request->body != NULL) {
        printf("Body: %s\n", request->body);
    }
    printf("\n\n");
}

void logRequest(struct Request *request) {

    size_t bodyLength = strlen(request->body == NULL ? "" : request->body);
    char *userAgent = getHeader(request->headers, "user-agent");
    char *referer = getHeader(request->headers, "referer");
    char *host = getHeader(request->headers, "host");
    char URL[REQUEST_PATH_MAX_SIZE];

    if (host != NULL) {
       sprintf(URL, "%s%s%s%s",request->scheme,"://", host, request->path);
    }else{
       strcpy(URL, "<URL>");
    }

    logInfo("Request | \"%s %s %s\" - %lu - %s - %s - %s - \"%s\"",
            methodToStr(request->method),
            request->path,
            request->protocolVersion,
            bodyLength,
            request->ip,
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
