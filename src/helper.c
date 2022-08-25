#include <ctype.h>  // for tolower()
#include <stdlib.h> // for malloc()

#include "helper.h"

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