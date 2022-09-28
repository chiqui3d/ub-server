#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "header.h"
#include "helper.h"

// add header
struct Header *addHeader(struct Header *header, char *name, char *value) {
    struct Header *newHeader = malloc(sizeof(struct Header));
    newHeader->name = strdup(name);
    newHeader->value =  strdup(value);

    if (header == NULL) {
        newHeader->next = NULL;
        header = newHeader;
    }else{       
        newHeader->next = header->next; 
        header->next = newHeader;
    }
   
    return header;
}

char *getHeader(struct Header *header, char *name) {
    while (header != NULL) {
        if (strcmp(header->name, name) == 0) {
            return header->value;
        }
        header = header->next;
    }
    return NULL;
}


void freeHeader(struct Header *header) {

    while (header != NULL) {
        struct Header *tmp = header;
        header = header->next;
        free(tmp->name);
        free(tmp->value);
        free(tmp);
    }

}

void printHeaders(struct Header *header) {
    while (header != NULL) {
        printf("%s: %s\n", header->name, header->value);
        header = header->next;
    }
}