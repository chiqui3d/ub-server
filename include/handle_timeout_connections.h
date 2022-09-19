#ifndef HANDLE_TIMEOUT_CONNECTIONS_H
#define HANDLE_TIMEOUT_CONNECTIONS_H

#include <time.h>
#include "server.h"

#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 1000
#endif

struct ConnectionTimeoutElement {
    time_t priorityTime;
    int fd; // file descriptor
};
struct QueueConnectionsTimeout {
    struct ConnectionTimeoutElement connections[MAX_CONNECTIONS];
    int currentSize;
    int capacity;
};

extern struct QueueConnectionsTimeout queueConnectionsTimeout;
extern int IndexQueueConnectionsFd[MAX_CONNECTIONS];



void createQueueConnections();
void enqueueConnection(struct ConnectionTimeoutElement connection);
void updateQueueConnection(int fd, time_t now);
void dequeueConnection();
void dequeueConnectionByFd(int fd);
struct ConnectionTimeoutElement peekQueueConnections();
void heapify(int index);
void swapConnectionElementHeap(struct ConnectionTimeoutElement *a, struct ConnectionTimeoutElement *b);
int leftChildHeap(int element);
int rightChildHeap(int element);
int parentHeap(int element);
void printQueueConnections();




#endif // END HANDLE_TIMEOUT_CONNECTIONS_H