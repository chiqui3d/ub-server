#ifndef QUEUE_CONNECTIONS_H
#define QUEUE_CONNECTIONS_H

#include <stddef.h> // for size_t
#include <time.h> // for time_t
#include <stdbool.h> // for bool

#include "server.h"
#include "http_status_code.h"


typedef enum Method { METHOD_GET,
                      METHOD_POST,
                      METHOD_PUT,
                      METHOD_PATCH,
                      METHOD_DELETE,
                      METHOD_HEAD,
                      METHOD_OPTIONS, 
                      METHOD_UNSUPPORTED } Method;

static const char *methodsList[] = { "GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS", "UNSUPPORTED" };

enum stateConnection {
    STATE_CONNECTION_RECV, // receive data
    STATE_CONNECTION_SEND_HEADERS, // send headers
    STATE_CONNECTION_SEND_BODY, // send body
    STATE_CONNECTION_DONE // done
};

struct QueueConnectionElementType {
    time_t priorityTime;
    int clientFd; // file descriptor
    enum stateConnection state;
    char *requestBuffer;
    size_t requestBufferLength;
    size_t requestBufferOffset;
    bool doneForClose;
    bool keepAlive;
    char *responseBufferHeaders;
    size_t responseBufferHeadersLength;
    size_t responseBufferHeadersOffset;
    int bodyFd;
    size_t bodyLength;
    off_t bodyOffset;
    char scheme[6]; // http or https
    char protocolVersion[9]; // HTTP/1.1
    enum Method method;
    char *path;
    char *absolutePath;
    struct Header *requestHeaders;
    char *requestBody;
    char ip[INET6_ADDRSTRLEN]; // IPv4 or IPv6
    enum HTTP_STATUS_CODE responseStatusCode;
};
struct QueueConnectionsType {
    int currentSize;
    int capacity;
    int indexQueue[MAX_EPOLL_EVENTS];
    struct QueueConnectionElementType connections[MAX_EPOLL_EVENTS];
};


struct QueueConnectionsType createQueueConnections();
void enqueueConnection(struct QueueConnectionsType *queueConnections, struct QueueConnectionElementType connection);
struct QueueConnectionElementType *getConnectionByFd(struct QueueConnectionsType *queueConnections, int fd);
void updateConnectionByFd(struct QueueConnectionsType *queueConnections, struct QueueConnectionElementType connection);
void updateQueueConnection(struct QueueConnectionsType *queueConnections, int fd);
void dequeueConnection(struct QueueConnectionsType *queueConnections);
void dequeueConnectionByFd(struct QueueConnectionsType *queueConnections, int fd);
struct QueueConnectionElementType peekQueueConnections(struct QueueConnectionsType *queueConnections);
void heapify(struct QueueConnectionsType *queueConnections, int index);
void swapConnectionElementHeap(struct QueueConnectionElementType *a, struct QueueConnectionElementType *b);
int leftChildHeap(int element);
int rightChildHeap(int element);
int parentHeap(int element);
void freeConnection(struct QueueConnectionElementType *connection);
struct QueueConnectionElementType emptyConnection();
void printConnection(struct QueueConnectionElementType connection);
void printQueueConnections(struct QueueConnectionsType *queueConnections);

#endif // END QUEUE_CONNECTIONS_H