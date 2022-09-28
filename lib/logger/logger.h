#ifndef LOGGER_H
#define LOGGER_H

#include <limits.h>  // for CHAR_MAX
#include <stdbool.h> // for bool
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdatomic.h>
#include <pthread.h>

#include "../color/color.h"

#define LOGGER_CURRENT_DATETIME_FORMAT "%Y-%m-%dT%H:%M:%S%z"
#define LOGGER_CURRENT_FILE_DATE_FORMAT "%Y-%m-%d"


#define DEFAULT_LOGGER_FILE_PATH "/var/log/ub-server/"
#define DEFAULT_LOGGER_OPEN_MODE "a+"
#define DEFAULT_LOGGER_DIRECTORY_PERMISSION_MODE 1755 // 1775 = 755 + 1000 Sticky bit
#define DEFAULT_LOGGER_PERMISSION_MODE 0755 // 0755 = 755 + 0000 Sticky bit
#define LOGGER_MESSAGE_MAX 1024
#define LOGGER_PATH_MAX 1024
#define LOGGER_FILE_NAME_MAX 15 // 2020-08-27.log 15 chars


#define LOGGER_DATE_FORMAT_MAX 11     // 10 + 1
#define LOGGER_DATETIME_FORMAT_MAX 26 // 25 + 1

enum LOG_LEVEL {
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
};

static const char *logLevelList[] = {"ERROR", "WARN", "INFO", "DEBUG"};

struct Logger {
    atomic_bool initialized;
    pthread_mutex_t lock;
    bool active;
    char path[LOGGER_PATH_MAX];
    char fileName[LOGGER_FILE_NAME_MAX];
    int fileFd;
};

extern struct Logger LOGGER;

const char *loggerLevelToStr(enum LOG_LEVEL level);
char *getLoggerFullPath(char *loggerFullPath);
char *getLoggerCurrentFullPath(char *loggerFullPath);
char *getLoggerFileName(char *fileName);

char *getLoggerFileCurrentDate(char *date);
char *getLoggerCurrentDatetime(char *datetime);
int getLoggerFileDescriptor();

int mkpath(const char *file_path, mode_t mode);
size_t writeAll(int fd, const void *buffer, size_t count);


void logMessage(enum LOG_LEVEL level, bool showErrno, char *codeFileName, int codeLine, char *message, ...);

#ifdef NDEBUG
#define logDebug(message, ...)
#else
#define logDebug(message, ...) logMessage(LOG_LEVEL_DEBUG, false, __FILE__, __LINE__, message, ##__VA_ARGS__)
#endif

#define logInfo(message, ...) logMessage(LOG_LEVEL_INFO, false, __FILE__, __LINE__, message, ##__VA_ARGS__)
#define logWarning(message, ...) logMessage(LOG_LEVEL_WARN, true, __FILE__, __LINE__, message, ##__VA_ARGS__)
#define logError(message, ...) logMessage(LOG_LEVEL_ERROR, true, __FILE__, __LINE__, message, ##__VA_ARGS__)

#endif // LOGGER_H