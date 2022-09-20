/**
 *
 * @brief Persistent connections timeout handler (HTTP/1.1 keep-alive)
 * Code to manage the queue of client connections through the min-heap data structure and an array
 * to store the index of the heap, in order to update the descriptor files, in case they need to update the time.
 * It may happen that they connect before they leave the queue and then the time needs to be reset.
 *
 * @version 0.1
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

#include "../lib/logger/logger.h"
#include "queue_connections.h"
#include "helper.h"

struct QueueConnectionsType QueueConnections;
int IndexQueueConnectionsFd[MAX_CONNECTIONS];

void createQueueConnections() {
    memset(QueueConnections.connections, 0, MAX_CONNECTIONS);
    QueueConnections.currentSize = 0;
    QueueConnections.capacity = (int)MAX_CONNECTIONS;
    memset(IndexQueueConnectionsFd, -1, MAX_CONNECTIONS);
}

void enqueueConnection(struct QueueConnectionElementType connection) {
    if (QueueConnections.currentSize == QueueConnections.capacity) {
        logWarning("Queue connection is full (max %d). The fd %d cannot be inserted\n", MAX_CONNECTIONS, connection.fd);
        return;
    }

    // insert in the last position
    QueueConnections.connections[QueueConnections.currentSize] = connection;
    QueueConnections.currentSize++;
    int indexQueue = QueueConnections.currentSize - 1; // last element
    // loop until the element is minor than its parent or it is the root
    // shiftUp
    while (indexQueue != 0 && difftime(connection.priorityTime, QueueConnections.connections[parentHeap(indexQueue)].priorityTime) < 0) {
        swapConnectionElementHeap(&QueueConnections.connections[indexQueue], &QueueConnections.connections[parentHeap(indexQueue)]);
        indexQueue = parentHeap(indexQueue);
    }
    logDebug(GREEN "Enqueue connection fd %d in the index %d" RESET, connection.fd, indexQueue);
    // assign the index of the heap to the array
    IndexQueueConnectionsFd[connection.fd] = indexQueue;
}

void updateQueueConnection(int fd, time_t now) {

    logDebug(RED "Update queue connection fd %d" RESET, fd);

    int index = IndexQueueConnectionsFd[fd];
    time_t oldPriorityTime = QueueConnections.connections[index].priorityTime;
    QueueConnections.connections[index].priorityTime = now;

    if (difftime(now, oldPriorityTime) < 0) {
        logDebug(RED "Shift up" RESET);
        // shiftUp
        while (index != 0 && difftime(now, QueueConnections.connections[parentHeap(index)].priorityTime) < 0) {
            swapConnectionElementHeap(&QueueConnections.connections[index], &QueueConnections.connections[parentHeap(index)]);
            index = parentHeap(index);
        }
        IndexQueueConnectionsFd[fd] = index;
    } else {
        logDebug(RED "Shift down" RESET);
        // shiftDown
        heapify(index);
    }
}

void dequeueConnection() {
    if (QueueConnections.currentSize == 0) {
        logDebug("Queue is empty");
        return;
    }
    logDebug("Dequeue connection");

    int fd0 = QueueConnections.connections[0].fd;
    int fdLast = QueueConnections.connections[QueueConnections.currentSize - 1].fd;
    // swap the last element with the first
    QueueConnections.connections[0] = QueueConnections.connections[QueueConnections.currentSize - 1];
    QueueConnections.connections[QueueConnections.currentSize - 1] = (struct QueueConnectionElementType){0, 0};
    // remove the last element
    QueueConnections.currentSize--;
    // remove the index of the heap from the array
    IndexQueueConnectionsFd[fd0] = -1;
    IndexQueueConnectionsFd[fdLast] = 0;
    // shiftDown
    heapify(0);
}

void dequeueConnectionByFd(int fd) {
    if (QueueConnections.currentSize == 0) {
        logDebug("Queue is empty");
        return;
    }
    logDebug("Dequeue connection fd %d", fd);
    int index = IndexQueueConnectionsFd[fd];
    if (index == -1) {
        logDebug(RED "The fd %d is not in the queue" RESET, fd);
        return;
    }
    int fdLast = QueueConnections.connections[QueueConnections.currentSize - 1].fd;
    // swap the last element with the first
    swapConnectionElementHeap(&QueueConnections.connections[index], &QueueConnections.connections[QueueConnections.currentSize - 1]);
    QueueConnections.connections[QueueConnections.currentSize - 1] = (struct QueueConnectionElementType){0, 0};
    // remove the last element
    QueueConnections.currentSize--;
    // remove the index of the heap from the array
    IndexQueueConnectionsFd[fd] = -1;
    IndexQueueConnectionsFd[fdLast] = index;
    // shiftDown
    heapify(index);
}

// Get the value of the front of the queue without removing it
struct QueueConnectionElementType peekQueueConnections() {
    if (QueueConnections.currentSize == 0) {
        logDebug("Queue is empty");
        return (struct QueueConnectionElementType){0, 0};
    }

    return QueueConnections.connections[0];
}

// shiftDown
void heapify(int index) {
    int left = leftChildHeap(index);
    int right = rightChildHeap(index);
    int smallest = index;

    if (left < QueueConnections.currentSize && difftime(QueueConnections.connections[left].priorityTime, QueueConnections.connections[smallest].priorityTime) < 0) {
        smallest = left;
    }
    if (right < QueueConnections.currentSize && difftime(QueueConnections.connections[right].priorityTime, QueueConnections.connections[smallest].priorityTime) < 0) {
        smallest = right;
    }
    if (smallest != index) { // if the smallest is not the current index, then swap
        swapConnectionElementHeap(&QueueConnections.connections[index], &QueueConnections.connections[smallest]);
        IndexQueueConnectionsFd[QueueConnections.connections[index].fd] = index;
        IndexQueueConnectionsFd[QueueConnections.connections[smallest].fd] = smallest;
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
void swapConnectionElementHeap(struct QueueConnectionElementType *a, struct QueueConnectionElementType *b) {
    struct QueueConnectionElementType temp = *a;
    *a = *b;
    *b = temp;
}

void printQueueConnections() {
    errno = 0;
    logDebug(RED "Size: %d, Capacity: %d" RESET, QueueConnections.currentSize, QueueConnections.capacity);
    int i;
    char date[20];
    for (i = 0; i < QueueConnections.currentSize; i++) {
        int index = IndexQueueConnectionsFd[QueueConnections.connections[i].fd];
        timeToDatetimeString(QueueConnections.connections[i].priorityTime, date);
        // We show both the queue data and the array that stores the queue indexes, 
        // to see if they match between updates and deletes.
        logDebug("index: %d, fd: %d, time: %ld, date: %s | index: %d, fd: %d, time: %ld",
               i,
               QueueConnections.connections[i].fd,
               QueueConnections.connections[i].priorityTime,
               date,
               index,
               QueueConnections.connections[index].fd,
               QueueConnections.connections[index].priorityTime);
    }
    if (QueueConnections.currentSize == 0) {
        logDebug(RED "Empty queue" RESET);
    }
    
}