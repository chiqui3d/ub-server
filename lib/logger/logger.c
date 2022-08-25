#include <errno.h>
#include <fcntl.h>
#include <fcntl.h> // for fcntl() nonblocking socket
#include <stdio.h>
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

    strcpy(loggerFullPath, LOGGER.path);
    strcat(loggerFullPath, LOGGER.fileName);

    return loggerFullPath;
}

char *getLoggerCurrentFullPath(char *loggerFullPath) {

    strcpy(loggerFullPath, LOGGER.path);

    char loggerFileName[LOGGER_FILE_NAME_MAX];

    strcat(loggerFullPath, getLoggerFileName(loggerFileName));

    return loggerFullPath;
}

char *getLoggerFileName(char *loggerFileName) {
    char loggerDateName[LOGGER_DATE_FORMAT_MAX];

    // sprintf(loggerFileName, "%s.log", date);
    getLoggerFileCurrentDate(loggerDateName);

    strcpy(loggerFileName, loggerDateName);
    strcat(loggerFileName, ".log");

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

    char loggerFileName[LOGGER_PATH_MAX];
    // check if the file name (%Y-%m-%d.log) is the same as the current one
    getLoggerFileName(loggerFileName);

    if (LOGGER.fileFd == -1 || strcmp(loggerFileName, LOGGER.fileName) != 0) {

        char loggerFullPath[LOGGER_PATH_MAX];
        getLoggerCurrentFullPath(loggerFullPath);
        if (mkpath(LOGGER.path, DEFAULT_LOGGER_PERMISSION_MODE) == -1) {
            die("Error creating directory: %s", LOGGER.path);
        }
        if (LOGGER.fileFd != -1) {
            close(LOGGER.fileFd);
        }

        strcpy(LOGGER.fileName, loggerFileName);
        // https://stackoverflow.com/questions/164053/should-log-file-streams-be-opened-closed-on-each-write-or-kept-open-during-a-des
        LOGGER.fileFd = open(loggerFullPath, O_APPEND | O_CREAT | O_WRONLY);
        if (LOGGER.fileFd == -1) {
            die("fopen %s", loggerFullPath);
        }
        // makeFileDescriptorNonBlocking(LOGGER.fileFd);
    }

    return LOGGER.fileFd;
}

void logMessage(enum LOG_LEVEL level, bool showErrno, char *codeFileName, int codeLine, char *message, ...) {

    char format[600];
    char messageWithArguments[200];
    char datetime[LOGGER_DATETIME_FORMAT_MAX];

    sprintf(format, "%s | %s | %s:%d", getLoggerCurrentDatetime(datetime), loggerLevelToStr(level), codeFileName, codeLine);

    va_list args;
    va_start(args, message);
    vsprintf(messageWithArguments, message, args);
    va_end(args);

    int fileFd = getLoggerFileDescriptor();
    if (fileFd == -1) {
        die("Not file descriptor found for logger file");
    }
    sprintf(format + strlen(format), " | %s", messageWithArguments);

    if (errno != 0 && showErrno) {
        sprintf(format + strlen(format), " | %s\n", strerror(errno));
    } else {
        strcat(format, "\n");
    }

    int send = write(fileFd, format, strlen(format) + 1);
    if (send == -1) {
        die("Error writing to logger file");
    }
}

int makeFileDescriptorNonBlocking(int fd) {
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