#ifndef REQUEST_H
#define REQUEST_H

#include <arpa/inet.h>

#include "header.h"

#define REQUEST_PATH_MAX_SIZE 4096

typedef enum Method { METHOD_GET,
                      METHOD_POST,
                      METHOD_PUT,
                      METHOD_PATCH,
                      METHOD_DELETE,
                      METHOD_HEAD,
                      METHOD_OPTIONS, 
                      METHOD_UNSUPPORTED } Method;

static const char *methodsList[] = { "GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS", "UNSUPPORTED" };

struct Request {
    enum Method method;
    char *path;
    char *absolutePath;
    char *protocolVersion;
    struct Header *headers;
    char *body;
    char ip[INET6_ADDRSTRLEN]; // IPv4 or IPv6
    char scheme[6]; // http or https
};


char *readRequest(char *buffer, int clientFd, bool *doneForClose);
struct Request *makeRequest(char *buffer, int clientFd);
void freeRequest(struct Request *request);

void printRequest(struct Request *request);
void logRequest(struct Request *request);

const char *methodToStr(enum Method method);
enum Method strToMethod(char *method);




#endif // REQUEST_H