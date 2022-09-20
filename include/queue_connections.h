#ifndef QUEUE_CONNECTIONS_H
#define QUEUE_CONNECTIONS_H

#include <time.h>
#include "server.h"

#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 1000
#endif

struct QueueConnectionElementType {
    time_t priorityTime;
    int fd; // file descriptor
};
struct QueueConnectionsType {
    struct QueueConnectionElementType connections[MAX_CONNECTIONS];
    int currentSize;
    int capacity;
};

extern struct QueueConnectionsType QueueConnections;
extern int IndexQueueConnectionsFd[MAX_CONNECTIONS];



void createQueueConnections();
void enqueueConnection(struct QueueConnectionElementType connection);
void updateQueueConnection(int fd, time_t now);
void dequeueConnection();
void dequeueConnectionByFd(int fd);
struct QueueConnectionElementType peekQueueConnections();
void heapify(int index);
void swapConnectionElementHeap(struct QueueConnectionElementType *a, struct QueueConnectionElementType *b);
int leftChildHeap(int element);
int rightChildHeap(int element);
int parentHeap(int element);
void printQueueConnections();




#endif // END QUEUE_CONNECTIONS_H