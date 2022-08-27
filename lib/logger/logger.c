#include <errno.h> // for errno
#include <fcntl.h> // for open() nonblocking socket
#include <stdio.h> // for sprintf()
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../die/die.h"
#include "./logger.h"

const char *loggerLevelToStr(enum LOG_LEVEL level) {
    return logLevelList[level];
}

char *getLoggerFileCurrentDate(char *date) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(date, LOGGER_DATE_FORMAT_MAX, LOGGER_CURRENT_FILE_DATE_FORMAT, tm);

    return date;
}

char *getLoggerCurrentDatetime(char *datetime) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(datetime, LOGGER_DATETIME_FORMAT_MAX, LOGGER_CURRENT_DATETIME_FORMAT, tm);

    return datetime;
}

char *getLoggerFullPath(char *loggerFullPath) {

    snprintf(loggerFullPath, LOGGER_PATH_MAX, "%s%s", LOGGER.path, LOGGER.fileName);

    return loggerFullPath;
}

char *getLoggerCurrentFullPath(char *loggerFullPath) {
    char loggerFileName[LOGGER_FILE_NAME_MAX];
    snprintf(loggerFullPath, LOGGER_PATH_MAX, "%s%s", LOGGER.path, getLoggerFileName(loggerFileName));

    return loggerFullPath;
}

char *getLoggerFileName(char *loggerFileName) {
    char loggerDate[LOGGER_DATE_FORMAT_MAX];
    snprintf(loggerFileName, LOGGER_FILE_NAME_MAX, "%s.log", getLoggerFileCurrentDate(loggerDate));

    return loggerFileName;
}

int mkpath(const char *file_path, mode_t mode) {

    // check if directory exists
    struct stat sb;
    if (stat(file_path, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        return 0;
    }

    // create directories

    // copy file path to tmp variable
    char tmp[CHAR_MAX];
    strcpy(tmp, file_path);

    size_t len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    char *p = NULL;
    // Adding one to a char to tmp places one character in front of the previous one.
    for (p = tmp + 1; *p; p++) {
        // printf("tmp:%s p: %s - %c\n",tmp, p, *p);
        if (*p == '/') {
            *p = 0; // terminate the string at the first '/' then for /var/log/ud-server we have /var when the p is at /log/ud-server
            // printf("TMP: %s\n", tmp);
            if (mkdir(tmp, mode) == -1 && errno != EEXIST) {
                return -1;
            }
            *p = '/'; // restore the '/'
        }
    }

    // we create the final directory of the path, because we remove the last '/'.
    if (mkdir(tmp, mode) == -1 && errno != EEXIST) {
        return -1;
    }

    return 0;
}

int getLoggerFileDescriptor() {

    if (LOGGER.active == false) {
        if (LOGGER.fileFd != STDERR_FILENO) {
            LOGGER.fileFd = STDERR_FILENO;
        }

        return LOGGER.fileFd;
    }

    char loggerFileName[LOGGER_FILE_NAME_MAX];
    getLoggerFileName(loggerFileName);

    // check if the file name (%Y-%m-%d.log) is the same as the current one
    if (LOGGER.fileFd == -1 || strncmp(loggerFileName, LOGGER.fileName, LOGGER_FILE_NAME_MAX) != 0) {

        char loggerFullPath[LOGGER_PATH_MAX];
        getLoggerCurrentFullPath(loggerFullPath);
        if (mkpath(LOGGER.path, DEFAULT_LOGGER_PERMISSION_MODE) == -1) {
            die("Error creating directory: %s", LOGGER.path);
        }
        if (LOGGER.fileFd != -1) {
            close(LOGGER.fileFd);
        }

        strncpy(LOGGER.fileName, loggerFileName, LOGGER_FILE_NAME_MAX);
        LOGGER.fileFd = open(loggerFullPath, O_APPEND | O_CREAT | O_WRONLY, DEFAULT_LOGGER_PERMISSION_MODE);
        if (LOGGER.fileFd == -1) {
            die("fopen %s", loggerFullPath);
        }
        // makeSocketNonBlocking(LOGGER.fileFd);
    }

    return LOGGER.fileFd;
}

void logMessage(enum LOG_LEVEL level, bool showErrno, char *codeFileName, int codeLine, char *message, ...) {

    char fullMessage[LOGGER_MESSAGE_MAX];
    char messageWithArguments[600];
    char datetime[LOGGER_DATETIME_FORMAT_MAX];

    va_list args;
    va_start(args, message);
    vsprintf(messageWithArguments, message, args);
    va_end(args);

    int fileFd = getLoggerFileDescriptor();
    if (fileFd == -1) {
        die("Not file descriptor found for logger file");
    }

    int lenFullMessage = snprintf(
        fullMessage,
        LOGGER_MESSAGE_MAX,
        "%s | %s | %s:%d | %s",
        getLoggerCurrentDatetime(datetime),
        loggerLevelToStr(level),
        codeFileName,
        codeLine,
        messageWithArguments);

    if (errno != 0 && showErrno) {
        int lenWithErrno = snprintf(fullMessage + lenFullMessage, LOGGER_MESSAGE_MAX, " | %s\n", strerror(errno));
        lenFullMessage += lenWithErrno;
    } else {
        fullMessage[lenFullMessage] = '\n';
        fullMessage[lenFullMessage + 1] = '\0';
        lenFullMessage++;
    }

   if (writeAll(fileFd, fullMessage, lenFullMessage) == -1) {
        die("Error writing to logger file");
    }
}

/**
 * @brief Write all the bytes of a buffer to a file descriptor, not writing more than necessary.
 *
 * @param fd
 * @param buffer
 * @param count
 *
 * @return size_t
 */
size_t writeAll(int fd, const void *buffer, size_t count) {
    size_t left_to_write = count;
    while (left_to_write > 0) {
        size_t written = write(fd, buffer, count);
        if (written == -1) {
            // An error occurred
            if (errno == EINTR) {
                // The call was interrupted by a signal
               return 0;
            } 
            return -1;
        } else {
            // Keep count of how much more we need to write.
            left_to_write -= written;
        }
    }
    // We should have written no more than COUNT bytes!
    // The number of bytes written is exactly COUNT
    return count;
}