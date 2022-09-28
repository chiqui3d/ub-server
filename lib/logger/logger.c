#include <aio.h>
#include <errno.h>
#include <fcntl.h> // for open() nonblocking socket
#include <stdarg.h>
#include <stdlib.h>
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
            *p = 0; // terminate the string at the first '/' then for /var/log/ub-server we have /var when the p is at /log/ub-server
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
        LOGGER.initialized = true;
        return STDERR_FILENO;
    }

    char newLoggerFileName[LOGGER_FILE_NAME_MAX];
    getLoggerFileName(newLoggerFileName);

    // check if the file name (%Y-%m-%d.log) is the same as the current one
    if (strncmp(newLoggerFileName, LOGGER.fileName, LOGGER_FILE_NAME_MAX) != 0) {
        LOGGER.initialized = false;
    }

    if (!LOGGER.initialized) {
        pthread_mutex_lock(&LOGGER.lock);
        if (!LOGGER.initialized) {
            char loggerFullPath[LOGGER_PATH_MAX];
            getLoggerCurrentFullPath(loggerFullPath);
            if (mkpath(LOGGER.path, DEFAULT_LOGGER_DIRECTORY_PERMISSION_MODE) == -1) {
                die("Error creating directory: %s", LOGGER.path);
            }

            close(LOGGER.fileFd);
            strncpy(LOGGER.fileName, newLoggerFileName, LOGGER_FILE_NAME_MAX);
            LOGGER.fileName[LOGGER_FILE_NAME_MAX - 1] = '\0';
            // TODO: check permissions if the file is already exists and if the permissions are not correct, remove and create a new file

            LOGGER.fileFd = open(loggerFullPath, O_APPEND | O_CREAT | O_WRONLY | O_NOFOLLOW, DEFAULT_LOGGER_PERMISSION_MODE);
            if (LOGGER.fileFd == -1) {
                die("Error open path %s", loggerFullPath);
            }
            LOGGER.initialized = true;
        }
        pthread_mutex_unlock(&LOGGER.lock);
    }

    return LOGGER.fileFd;
}

void logMessage(enum LOG_LEVEL level, bool showErrno, char *codeFileName, int codeLine, char *message, ...) {

    int fileFd = getLoggerFileDescriptor();
    if (fileFd == -1) {
        die("Not file descriptor found for logger file");
    }

    va_list args;
    va_start(args, message);
    int lenMessageWithArguments = vsnprintf(NULL, 0, message, args);
    va_end(args);

    lenMessageWithArguments++; // add 1 for the null terminator

    char messageWithArguments[lenMessageWithArguments];
    va_start(args, message);
    vsnprintf(messageWithArguments, lenMessageWithArguments, message, args);
    va_end(args);

    char datetime[LOGGER_DATETIME_FORMAT_MAX];
    getLoggerCurrentDatetime(datetime);
    const char *levelName = loggerLevelToStr(level);

    int lenFullMessage = snprintf(
        NULL,
        0,
        "%s | %s | %s:%d | %s",
        datetime,
        levelName,
        codeFileName,
        codeLine,
        messageWithArguments);

    if (errno != 0 && showErrno) {
        lenFullMessage += snprintf(NULL, 0, " | %s", strerror(errno));
    }

    lenFullMessage += 1; // add 1 for the new line

    char fullMessage[lenFullMessage];
    int offset = snprintf(fullMessage, lenFullMessage, "%s | %s | %s:%d | %s", datetime, levelName, codeFileName, codeLine, messageWithArguments);

    if (errno != 0 && showErrno) {
        snprintf(fullMessage + offset, lenFullMessage - offset, " | %s", strerror(errno));
    }
    fullMessage[lenFullMessage - 1] = '\n';

    // TODO: truncate message if it is too long (lenFullMessage > LOGGER_MAX_MESSAGE_LENGTH)

    struct aiocb *aiocbp;
    aiocbp = malloc(sizeof(struct aiocb));
    memset(aiocbp, 0, sizeof(struct aiocb));
    aiocbp->aio_fildes = fileFd;
    aiocbp->aio_buf = fullMessage;
    aiocbp->aio_nbytes = lenFullMessage;
    aiocbp->aio_sigevent.sigev_notify = SIGEV_NONE;

    if (aio_write(aiocbp) == -1) {
        die("aio_write: Error writing to logger file");
    }

    while (aio_error(aiocbp) == EINPROGRESS) {
        // wait until the write is done
    }
    int loggerWriteResult = aio_return(aiocbp);
    free(aiocbp);
    if (loggerWriteResult == -1) {
        fprintf(stderr, "%s", "aio_return: Error writing to logger file");
    }

   /*  if (writeAll(fileFd, fullMessage, lenFullMessage) == -1) {
        die("Error writing to logger file");
    } */
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
            if (errno == EINTR) {
                // The call was interrupted by a signal
               continue;
            }
            return -1;
        } else {
            left_to_write -= written;
        }
    }
    return count;
}