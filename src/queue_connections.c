/**
 *
 * @brief Persistent connections queue by time (HTTP/1.1 keep-alive)
 *
 * Code to manage the queue of client connections through the min-heap data structure and an array
 * to store the index of the heap, in order to update the descriptor files, in case they need to update the time.
 * It may happen that they connect before they leave the queue and then the time needs to be updated.
 *
 * @version 0.2
 * @author chiqui3d
 * @date 2022-09-17
 *
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "queue_connections.h"
#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "header.h"
#include "helper.h"


struct QueueConnectionsType createQueueConnections() {
    struct QueueConnectionsType queueConnections;
    memset(queueConnections.connections, 0, MAX_EPOLL_EVENTS * sizeof(struct QueueConnectionElementType));
    queueConnections.currentSize = 0;
    queueConnections.capacity = (int)MAX_EPOLL_EVENTS;
    memset(queueConnections.indexQueue, -1, MAX_EPOLL_EVENTS * sizeof(int));
    return queueConnections;
}

void enqueueConnection(struct QueueConnectionsType *queueConnections, struct QueueConnectionElementType connection) {
    if (queueConnections->currentSize == queueConnections->capacity) {
        logWarning("Queue connection is full (max %d). The fd %d cannot be inserted\n", MAX_EPOLL_EVENTS, connection.clientFd);
        return;
    }

    // insert in the last position
    queueConnections->connections[queueConnections->currentSize] = connection;
    queueConnections->currentSize++;
    int indexQueue = queueConnections->currentSize - 1; // last element
    // loop until the element is minor than its parent or it is the root
    // shiftUp
    while (indexQueue != 0 && difftime(connection.priorityTime, queueConnections->connections[parentHeap(indexQueue)].priorityTime) < 0) {
        swapConnectionElementHeap(&queueConnections->connections[indexQueue], &queueConnections->connections[parentHeap(indexQueue)]);
        indexQueue = parentHeap(indexQueue);
    }
    logDebug("Enqueue connection fd %d in the index %d", connection.clientFd, indexQueue);
    // assign the index of the heap to the array
    queueConnections->indexQueue[connection.clientFd] = indexQueue;
}

struct QueueConnectionElementType *getConnectionByFd(struct QueueConnectionsType *queueConnections, int fd) {

    int index = queueConnections->indexQueue[fd];
    if (index == -1) {
        struct QueueConnectionElementType connection = emptyConnection();
        connection.clientFd = fd;
        connection.priorityTime = time(NULL);
        connection.state = STATE_CONNECTION_RECV;
        connection.requestBuffer = malloc(sizeof(char) * BUFFER_REQUEST_SIZE);
        connection.requestBufferLength = BUFFER_REQUEST_SIZE;
        enqueueConnection(queueConnections, connection);
        index = queueConnections->indexQueue[fd];
        if (index == -1) {
            die("getConnectionByFd: bad queue index");
        }
    }

    return &queueConnections->connections[index];
}

void updateConnectionByFd(struct QueueConnectionsType *queueConnections, struct QueueConnectionElementType connection) {

    int index = queueConnections->indexQueue[connection.clientFd];
    if (index == -1) {
        die("updateConnectionByFd: bad queue index");
    }
    // update the connection
    queueConnections->connections[index] = connection;
}

void updateQueueConnection(struct QueueConnectionsType *queueConnections, int fd) {

    logDebug("Update queue connection fd %d", fd);

    int index = queueConnections->indexQueue[fd];
    time_t oldPriorityTime = queueConnections->connections[index].priorityTime;
    time_t newPriorityTime = time(NULL);

    freeConnection(&queueConnections->connections[index]);
    queueConnections->connections[index] = emptyConnection();
    queueConnections->connections[index].clientFd = fd;
    queueConnections->connections[index].priorityTime = newPriorityTime;
    queueConnections->connections[index].state = STATE_CONNECTION_RECV;
    queueConnections->connections[index].requestBuffer = malloc(sizeof(char) * BUFFER_REQUEST_SIZE);
    queueConnections->connections[index].requestBufferLength = BUFFER_REQUEST_SIZE;

    if (difftime(newPriorityTime, oldPriorityTime) < 0) {
        // shiftUp
        while (index != 0 && difftime(newPriorityTime, queueConnections->connections[parentHeap(index)].priorityTime) < 0) {
            swapConnectionElementHeap(&queueConnections->connections[index], &queueConnections->connections[parentHeap(index)]);
            index = parentHeap(index);
        }
        queueConnections->indexQueue[fd] = index;
    } else {
        // shiftDown
        heapify(queueConnections, index);
    }
}

void dequeueConnection(struct QueueConnectionsType *queueConnections) {
    if (queueConnections->currentSize == 0) {
        logDebug("Queue is empty");
        return;
    }
    int fdFirst = queueConnections->connections[0].clientFd;
    int fdLast = queueConnections->connections[queueConnections->currentSize - 1].clientFd;
    logDebug("Dequeue connection fd %d", fdFirst);

    freeConnection(&queueConnections->connections[0]);

    if (fdFirst == fdLast) {
        queueConnections->connections[0] = emptyConnection();
        queueConnections->indexQueue[fdFirst] = -1;
        queueConnections->currentSize--;
        return;
    }
    // swap the last element with the first
    queueConnections->connections[0] = queueConnections->connections[queueConnections->currentSize - 1];
    queueConnections->connections[queueConnections->currentSize - 1] = emptyConnection();
    // remove the last element
    queueConnections->currentSize--;
    // remove the index of the heap from the array
    queueConnections->indexQueue[fdFirst] = -1;
    queueConnections->indexQueue[fdLast] = 0;
    // shiftDown
    heapify(queueConnections, 0);
}

void dequeueConnectionByFd(struct QueueConnectionsType *queueConnections, int fd) {
    if (queueConnections->currentSize == 0) {
        logDebug("Queue is empty");
        return;
    }
    logDebug("Dequeue connection fd %d", fd);
    int index = queueConnections->indexQueue[fd];

    if (index == -1) {
        logDebug("The fd %d is not in the queue", fd);
        return;
    }
    freeConnection(&queueConnections->connections[index]);
    int fdLast = queueConnections->connections[queueConnections->currentSize - 1].clientFd;
    if (fdLast == fd) {
        logDebug("The fd %d is the last element of the queue", fd);
        queueConnections->connections[queueConnections->currentSize - 1] = emptyConnection();
        queueConnections->currentSize--;
        queueConnections->indexQueue[fd] = -1;
        return;
    }
    // swap the last element with the first
    swapConnectionElementHeap(&queueConnections->connections[index], &queueConnections->connections[queueConnections->currentSize - 1]);
    queueConnections->connections[queueConnections->currentSize - 1] = emptyConnection();
    // remove the last element
    queueConnections->currentSize--;
    // remove the index of the heap from the array
    queueConnections->indexQueue[fd] = -1;
    queueConnections->indexQueue[fdLast] = index;
    // shiftDown
    heapify(queueConnections, index);
}

// Get the value of the front of the queue without removing it
struct QueueConnectionElementType peekQueueConnections(struct QueueConnectionsType *queueConnections) {
    if (queueConnections->currentSize == 0) {
        logDebug("Queue is empty");
        return emptyConnection();
    }

    return queueConnections->connections[0];
}

// shiftDown
void heapify(struct QueueConnectionsType *queueConnections, int index) {
    int left = leftChildHeap(index);
    int right = rightChildHeap(index);
    int smallest = index;

    if (left < queueConnections->currentSize && difftime(queueConnections->connections[left].priorityTime, queueConnections->connections[smallest].priorityTime) < 0) {
        smallest = left;
    }
    if (right < queueConnections->currentSize && difftime(queueConnections->connections[right].priorityTime, queueConnections->connections[smallest].priorityTime) < 0) {
        smallest = right;
    }
    if (smallest != index) { // if the smallest is not the current index, then swap
        swapConnectionElementHeap(&queueConnections->connections[index], &queueConnections->connections[smallest]);
        queueConnections->indexQueue[queueConnections->connections[index].clientFd] = index;
        queueConnections->indexQueue[queueConnections->connections[smallest].clientFd] = smallest;
        heapify(queueConnections, smallest);
    }
}

int leftChildHeap(int element) {
    return 2 * element + 1;
}

int rightChildHeap(int element) {
    return 2 * element + 2;
}

int parentHeap(int element) {
    return (element - 1) / 2;
}
void swapConnectionElementHeap(struct QueueConnectionElementType *a, struct QueueConnectionElementType *b) {
    struct QueueConnectionElementType temp = *a;
    *a = *b;
    *b = temp;
}

void freeConnection(struct QueueConnectionElementType *connection) {

    if (connection->bodyFd > 0) {
        close(connection->bodyFd);
    }
    if (connection->path != NULL) {
        free(connection->path);
    }
    if (connection->absolutePath != NULL) {
        free(connection->absolutePath);
    }
    if (connection->requestBuffer != NULL) {
        free(connection->requestBuffer);
    }
    if (connection->responseBufferHeaders != NULL) {
        free(connection->responseBufferHeaders);
    }
    if (connection->requestHeaders != NULL) {
        freeHeader(connection->requestHeaders);
    }
    if (connection->requestBody != NULL) {
        free(connection->requestBody);
    }
}

struct QueueConnectionElementType emptyConnection() {
    struct QueueConnectionElementType connection;
    memset(&connection, 0, sizeof(struct QueueConnectionElementType));
    connection.clientFd = 0;
    connection.bodyFd = -1;
    connection.requestBuffer = NULL;
    connection.requestBufferLength = 0;
    connection.requestBufferOffset = 0;
    connection.state = STATE_CONNECTION_RECV;
    connection.requestBody = NULL;
    connection.requestHeaders = NULL;
    connection.responseBufferHeaders = NULL;
    connection.responseBufferHeadersLength = 0;
    connection.responseBufferHeadersOffset = 0;
    connection.path = NULL;
    connection.absolutePath = NULL;
    connection.keepAlive = false;
    connection.method = METHOD_GET;
    connection.responseStatusCode = HTTP_STATUS_OK;

    return connection;
}

void printConnection(struct QueueConnectionElementType connection) {
    errno = 0;
    char date[20];
    timeToDatetimeString(connection.priorityTime, date);

    logDebug("Connection fd %d,"
             " bodyFd %d,"
             " state %d,"
             " keepAlive %d,"
             " method %d,"
             " responseStatusCode %d,"
             " Date %s",
             connection.clientFd,
             connection.bodyFd,
             connection.state,
             connection.keepAlive,
             connection.method,
             connection.responseStatusCode,
             date);
}

void printQueueConnections(struct QueueConnectionsType *queueConnections) {
    errno = 0;
    logDebug("Current Size: %d, Capacity: %d", queueConnections->currentSize, queueConnections->capacity);
    int i;
    char date[20];
    for (i = 0; i < queueConnections->currentSize; i++) {
        int index = queueConnections->indexQueue[queueConnections->connections[i].clientFd];
        if (index != -1) {
            timeToDatetimeString(queueConnections->connections[i].priorityTime, date);
            // We show both the queue data and the array that stores the queue indexes,
            // to see if they match between updates and deletes.
            logDebug("index: %d, fd: %d, time: %ld, date: %s | index: %d, fd: %d, time: %ld",
                     i,
                     queueConnections->connections[i].clientFd,
                     queueConnections->connections[i].priorityTime,
                     date,
                     index,
                     queueConnections->connections[index].clientFd,
                     queueConnections->connections[index].priorityTime);
        }
    }
    if (queueConnections->currentSize == 0) {
        logDebug("Empty queue");
    }
}