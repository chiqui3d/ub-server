#ifndef RESPONSE_H
#define RESPONSE_H

#include <sys/types.h> // for size_t
#include <time.h>      // for strftime() and time_t

#include "header.h"
#include "http_status_code.h"
#include "request.h"

struct Response {
    char *protocolVersion;
    char *absolutePath;
    enum HTTP_STATUS_CODE statusCode;
    struct Header *headers;
    size_t size;
    time_t lastModified;
    int bodyFd;
};

void printResponse(struct Response *response);

void freeResponse(struct Response *response);

struct Response *makeResponse(struct Request *request, char *htmlDir);

void sendResponse(struct Response *response, int clientFd);

size_t sendAll(int fd, const void *buffer, size_t count);

void helloResponse(int clientFd);
void unsupportedProtocolResponse(int clientFd, char *protocolVersion);
void tooManyRequestResponse(int clientFd);

static char *helloResponseTemplate =
    "HTTP/1.1 200 OK\n"
    "Content-type: text/html; charset=UTF-8\n"
    "Content-Length: 56\n"
    "Connection: close\n"
    "\n"
    "<html>\n"
    " <body>\n"
    "  <h1>Hello world!</h1>\n"
    " </body>\n"
    "</html>\n";

static char *badRequestResponseTemplate =
    "HTTP/1.1 400 Bad Request\n"
    "Content-type: text/html; charset=UTF-8\n"
    "Connection: close\n"
    "\n"
    "<html>\n"
    " <body>\n"
    "  <h1>Bad Request</h1>\n"
    "  <p>This server did not understand your request.</p>\n"
    " </body>\n"
    "</html>\n";

static char *versionNotSupportedResponseTemplate =
    "HTTP/1.1 505 HTTP Version Not Supported\n"
    "Content-type: text/html; charset=UTF-8\n"
    "Connection: close\n"
    "\n"
    "<html>\n"
    " <body>\n"
    "  <h1>HTTP Version <%s> Not Supported</h1>\n"
    " </body>\n"
    "</html>\n";

static char *tooManyRequestResponseTemplate =
    "HTTP/1.1 429 Too Many Requests\n"
    "Content-type: text/html; charset=UTF-8\n"
    "Connection: close\n"
    "\n"
    "<html>\n"
    " <body>\n"
    "  <h1>Too Many Requests</h1>\n"
    " </body>\n"
    "</html>\n";

static char *notFoundResponseTemplate =
    "HTTP/1.1 404 Not Found\n"
    "Content-type: text/html; charset=UTF-8\n"
    "Connection: close\n"
    "\n"
    "<html>\n"
    " <body>\n"
    "  <h1>Not Found :(</h1>\n"
    "  <p>The requested URL %s was not found on this server.</p>\n"
    " </body>\n"
    "</html>\n";


static char *internalServerResponseTemplate =
    "HTTP/1.1 500 Internal Server Error\n"
    "Content-type: text/html; charset=UTF-8\n"
    "Connection: close\n"
    "\n"
    "<html>\n"
    " <body>\n"
    "  <h1>500 Internal Server Error</h1>\n"
    "  <p></p>\n"
    " </body>\n"
    "</html>\n";

#endif // RESPONSE_H