#ifndef QUEUE_CONNECTIONS_H
#define QUEUE_CONNECTIONS_H

#include "server.h"
#include <time.h>

#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 1024
#endif

struct QueueConnectionElementType {
    time_t priorityTime;
    int fd; // file descriptor
};
struct QueueConnectionsType {
    struct QueueConnectionElementType connections[MAX_CONNECTIONS];
    int currentSize;
    int capacity;
    int indexQueue[MAX_CONNECTIONS];
};

struct QueueConnectionsType createQueueConnections();
void enqueueConnection(struct QueueConnectionsType *queueConnections, struct QueueConnectionElementType connection);
void updateQueueConnection(struct QueueConnectionsType *queueConnections, int fd, time_t now);
void dequeueConnection(struct QueueConnectionsType *queueConnections);
void dequeueConnectionByFd(struct QueueConnectionsType *queueConnections, int fd);
struct QueueConnectionElementType peekQueueConnections(struct QueueConnectionsType *queueConnections);
void heapify(struct QueueConnectionsType *queueConnections, int index);
void swapConnectionElementHeap(struct QueueConnectionElementType *a, struct QueueConnectionElementType *b);
int leftChildHeap(int element);
int rightChildHeap(int element);
int parentHeap(int element);
void printQueueConnections(struct QueueConnectionsType *queueConnections);

#endif // END QUEUE_CONNECTIONS_H