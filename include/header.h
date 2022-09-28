#ifndef HEADER_H
#define HEADER_H

struct Header {
    char *name;
    char *value;
    struct Header *next;
};

struct Header *addHeader(struct Header *header, char *name, char *value);
char *getHeader(struct Header *header, char *name);
void freeHeader(struct Header *header);
void printHeaders(struct Header *header);

#endif // HEADER_H