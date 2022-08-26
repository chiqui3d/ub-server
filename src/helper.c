#include <ctype.h>  // for tolower()
#include <stdlib.h> // for malloc()
#include <fcntl.h>  // for fcntl() nonblocking socket
#include <stdio.h> // for perror()
#include "helper.h"

int makeSocketNonBlocking(int fd) {
    int flags, s;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(fd, F_SETFL, flags);
    if (s == -1) {
        perror("fcntl");
        return -1;
    }

    return 0;
}


char *toLower(char *str, size_t len) {

    char *strLower = calloc(len + 1, sizeof(char));

    for (size_t i = 0; i < len; i++) {
        strLower[i] = tolower((unsigned char)str[i]);
    }
    return strLower;
}
char *toUpper(char *str, size_t len) {
    char *strUpper = calloc(len + 1, sizeof(char));

    for (size_t i = 0; i < len; i++) {
        strUpper[i] = toupper((unsigned char)str[i]);
    }
    return strUpper;
}