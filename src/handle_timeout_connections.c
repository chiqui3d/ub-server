/**
 * @file handle_timeout_connections.c
 * @author chiqui3d
 *
 * @brief
 * Code to manage the queue of client connections through the min-heap data structure and an array
 * to store the index of the heap, in order to update the descriptor files, in case they need to update the time.
 * It may happen that they connect before they leave the queue and then the time needs to be reset.
 *
 * @version 0.1
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

#include "../lib/logger/logger.h"
#include "handle_timeout_connections.h"
#include "helper.h"

struct QueueConnectionsTimeout queueConnectionsTimeout;
int indexQueueConnectionsFd[MAX_CONNECTIONS];

void createQueueConnections() {
    memset(queueConnectionsTimeout.connections, 0, MAX_CONNECTIONS);
    queueConnectionsTimeout.currentSize = 0;
    queueConnectionsTimeout.capacity = (int)MAX_CONNECTIONS;
    memset(indexQueueConnectionsFd, -1, MAX_CONNECTIONS);
}

void enqueueConnection(struct ConnectionTimeoutElement connection) {
    if (queueConnectionsTimeout.currentSize == queueConnectionsTimeout.capacity) {
        logWarning("Queue connection is full (max %d). The fd %d cannot be inserted\n", MAX_CONNECTIONS, connection.fd);
        return;
    }

    // insert in the last position
    queueConnectionsTimeout.connections[queueConnectionsTimeout.currentSize] = connection;
    queueConnectionsTimeout.currentSize++;
    int indexQueue = queueConnectionsTimeout.currentSize - 1; // last element
    // loop until the element is minor than its parent or it is the root
    // shiftUp
    while (indexQueue != 0 && difftime(connection.priorityTime, queueConnectionsTimeout.connections[parentHeap(indexQueue)].priorityTime) < 0) {
        swapConnectionElementHeap(&queueConnectionsTimeout.connections[indexQueue], &queueConnectionsTimeout.connections[parentHeap(indexQueue)]);
        indexQueue = parentHeap(indexQueue);
    }
    logDebug(GREEN "Enqueue connection fd %d in the index %d" RESET, connection.fd, indexQueue);
    // assign the index of the heap to the array
    indexQueueConnectionsFd[connection.fd] = indexQueue;
}

void updateQueueConnection(int fd, time_t now) {

    logDebug(RED "Update queue connection fd %d" RESET, fd);

    int index = indexQueueConnectionsFd[fd];
    time_t oldPriorityTime = queueConnectionsTimeout.connections[index].priorityTime;
    queueConnectionsTimeout.connections[index].priorityTime = now;

    if (difftime(now, oldPriorityTime) < 0) {
        logDebug(RED "Shift up" RESET);
        // shiftUp
        while (index != 0 && difftime(now, queueConnectionsTimeout.connections[parentHeap(index)].priorityTime) < 0) {
            swapConnectionElementHeap(&queueConnectionsTimeout.connections[index], &queueConnectionsTimeout.connections[parentHeap(index)]);
            index = parentHeap(index);
        }
        indexQueueConnectionsFd[fd] = index;
    } else {
        logDebug(RED "Shift down" RESET);
        // shiftDown
        heapify(index);
    }
}

void dequeueConnection() {
    if (queueConnectionsTimeout.currentSize == 0) {
        logDebug("Queue is empty");
        return;
    }
    logDebug("Dequeue connection");

    int fd0 = queueConnectionsTimeout.connections[0].fd;
    int fdLast = queueConnectionsTimeout.connections[queueConnectionsTimeout.currentSize - 1].fd;
    // swap the last element with the first
    queueConnectionsTimeout.connections[0] = queueConnectionsTimeout.connections[queueConnectionsTimeout.currentSize - 1];
    queueConnectionsTimeout.connections[queueConnectionsTimeout.currentSize - 1] = (struct ConnectionTimeoutElement){0, 0};
    // remove the last element
    queueConnectionsTimeout.currentSize--;
    // remove the index of the heap from the array
    indexQueueConnectionsFd[fd0] = -1;
    indexQueueConnectionsFd[fdLast] = 0;
    // shiftDown
    heapify(0);
}

void dequeueConnectionByFd(int fd) {
    if (queueConnectionsTimeout.currentSize == 0) {
        logDebug("Queue is empty");
        return;
    }
    logDebug("Dequeue connection fd %d", fd);
    int index = indexQueueConnectionsFd[fd];
    if (index == -1) {
        logDebug(RED "The fd %d is not in the queue" RESET, fd);
        return;
    }
    int fdLast = queueConnectionsTimeout.connections[queueConnectionsTimeout.currentSize - 1].fd;
    // swap the last element with the first
    swapConnectionElementHeap(&queueConnectionsTimeout.connections[index], &queueConnectionsTimeout.connections[queueConnectionsTimeout.currentSize - 1]);
    queueConnectionsTimeout.connections[queueConnectionsTimeout.currentSize - 1] = (struct ConnectionTimeoutElement){0, 0};
    // remove the last element
    queueConnectionsTimeout.currentSize--;
    // remove the index of the heap from the array
    indexQueueConnectionsFd[fd] = -1;
    indexQueueConnectionsFd[fdLast] = index;
    // shiftDown
    heapify(index);
}

// Get the value of the front of the queue without removing it
struct ConnectionTimeoutElement peekQueueConnections() {
    if (queueConnectionsTimeout.currentSize == 0) {
        logDebug("Queue is empty");
        return (struct ConnectionTimeoutElement){0, 0};
    }

    return queueConnectionsTimeout.connections[0];
}

// shiftDown
void heapify(int index) {
    int left = leftChildHeap(index);
    int right = rightChildHeap(index);
    int smallest = index;

    if (left < queueConnectionsTimeout.currentSize && difftime(queueConnectionsTimeout.connections[left].priorityTime, queueConnectionsTimeout.connections[smallest].priorityTime) < 0) {
        smallest = left;
    }
    if (right < queueConnectionsTimeout.currentSize && difftime(queueConnectionsTimeout.connections[right].priorityTime, queueConnectionsTimeout.connections[smallest].priorityTime) < 0) {
        smallest = right;
    }
    if (smallest != index) { // if the smallest is not the current index, then swap
        swapConnectionElementHeap(&queueConnectionsTimeout.connections[index], &queueConnectionsTimeout.connections[smallest]);
        indexQueueConnectionsFd[queueConnectionsTimeout.connections[index].fd] = index;
        indexQueueConnectionsFd[queueConnectionsTimeout.connections[smallest].fd] = smallest;
        heapify(smallest);
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
void swapConnectionElementHeap(struct ConnectionTimeoutElement *a, struct ConnectionTimeoutElement *b) {
    struct ConnectionTimeoutElement temp = *a;
    *a = *b;
    *b = temp;
}

void printQueueConnections() {
    errno = 0;
    logDebug(RED "Size: %d, Capacity: %d" RESET, queueConnectionsTimeout.currentSize, queueConnectionsTimeout.capacity);
    int i;
    char date[20];
    for (i = 0; i < queueConnectionsTimeout.currentSize; i++) {
        int index = indexQueueConnectionsFd[queueConnectionsTimeout.connections[i].fd];
        timeToDatetimeString(queueConnectionsTimeout.connections[i].priorityTime, date);
        logDebug("index: %d, fd: %d, time: %ld, date: %s | index: %d, fd: %d, time: %ld",
               i,
               queueConnectionsTimeout.connections[i].fd,
               queueConnectionsTimeout.connections[i].priorityTime,
               date,
               index,
               queueConnectionsTimeout.connections[index].fd,
               queueConnectionsTimeout.connections[index].priorityTime);
    }
    if (queueConnectionsTimeout.currentSize == 0) {
        logDebug(RED "Empty queue" RESET);
    }
    
}