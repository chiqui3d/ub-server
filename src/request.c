#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/color/color.h"
#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "request.h"
#include "helper.h"


// https://stackoverflow.com/questions/15433188/what-is-the-difference-between-r-n-r-and-n
// https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response
// https://www.rfc-editor.org/rfc/rfc2616#section-2.2
// https://stackoverflow.com/questions/6686261/what-at-the-bare-minimum-is-required-for-an-http-request
// curl -X POST localhost:3001 --header "Content-Type: application/json" --header 'cache-control: no-cache' --data '{"productId": 123456, "quantity": 100}'
// curl -X POST localhost:3001 --header "Content-Type: text/plain" --header 'cache-control: no-cache' --data-binary "Yes23"
// https://reqbin.com/req/python/c-d2nzjn3z/curl-post-body#:~:text=You%20can%20pass%20the%20body,%2Dbinary%20command%2Dline%20option.

// https://stackoverflow.com/questions/8739189/search-for-a-newline-charactor-or-cr-lf-characters
// https://stackoverflow.com/questions/5757290/http-header-line-break-style
// https://stackoverflow.com/questions/30472018/how-to-extract-data-from-http-header-in-c
// https://www.jmarshall.com/easy/http/#http1.1c1
// https://stackoverflow.com/questions/26811822/is-it-necessary-to-check-both-r-n-r-n-and-n-n-as-http-header-content-separ
// https://stackoverflow.com/questions/8786084/how-to-parse-http-headers-in-c
// https://stackoverflow.com/questions/6324167/do-browsers-send-r-n-or-n-or-does-it-depend-on-the-browser
// https://stackoverflow.com/questions/14926062/detecting-end-of-http-header-with-r-n-r-n
// https://stackoverflow.com/questions/16243118/differ-between-header-and-content-of-http-server-response-sockets
// https://stackoverflow.com/questions/67509709/is-recvbufsize-guaranteed-to-receive-all-the-data-if-sended-data-is-smaller-th
// https://stackoverflow.com/questions/16990746/isnt-recv-in-c-socket-programming-blocking

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

void freeRequest(struct Request *request) {
    
    free(request->path);
    free(request->protocolVersion);
    if (request->body != NULL) {
        free(request->body);
    }
    
    freeHeader(request->headers);
    free(request);
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

    char method[10], path[100], protocolVersion[10];

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
