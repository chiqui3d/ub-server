#ifndef HELPER_H
#define HELPER_H

#include <stddef.h> // for size_t

#ifdef NDEBUG
#define IS_DEBUG 0
#else
#define IS_DEBUG 1
#endif

char *toLower(char *str, size_t len);
char *toUpper(char *str, size_t len);

#endif // HELPER_H