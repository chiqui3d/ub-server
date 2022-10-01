#ifndef HELPER_H
#define HELPER_H

#include <stddef.h> // for size_t
#include <time.h>

#ifdef NDEBUG
#define IS_DEBUG 0
#else
#define IS_DEBUG 1
#endif

#define DATETIME_HELPER_FORMAT "%Y-%m-%d %H:%M:%S"
#define DATETIME_HELPER_SIZE 20

int makeSocketNonBlocking(int fd);
char *timeToDatetimeString(time_t time, char *format);

void strCopySafe(char *dest, char *src);
char *toLower(char *str, size_t len);
char *toUpper(char *str, size_t len);

int makeDirectory(const char *file_path, mode_t mode);

char *readAll(int fd, char *buffer, size_t bufferSize);
size_t sendAll(int fd, char *buffer, size_t count);

#endif // HELPER_H