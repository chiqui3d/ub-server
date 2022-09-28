#ifndef REQUEST_H
#define REQUEST_H

#include <arpa/inet.h>
#include <stdbool.h>

#include "header.h"
#include "queue_connections.h"


bool isRequestComplete(char *buffer);
void recvRequest(struct QueueConnectionElementType *connection);
bool processRequest(struct QueueConnectionElementType *connection);

void printRequest(struct QueueConnectionElementType connection);
void logRequest(struct QueueConnectionElementType connection);

const char *methodToStr(enum Method method);
enum Method strToMethod(char *method);




#endif // REQUEST_H