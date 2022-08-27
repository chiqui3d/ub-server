#include <errno.h>        // for errno
#include <fcntl.h>        // for open()
#include <magic.h>        // for magic_open() mime type detection
#include <stdbool.h>      // for bool()
#include <stdio.h>        // for sprintf()
#include <string.h>       // for strlen()
#include <sys/sendfile.h> // for sendfile()
#include <sys/socket.h>   // for send()
#include <unistd.h>       // for close()

#include "../lib/die/die.h"
#include "../lib/logger/logger.h"
#include "response.h"
#include "server.h"

void printResponse(struct Response *response) {
    printf("Response:\n");
    printf("\tStatus: %d\n", response->statusCode);
    printf("\tAbsolute path: %s\n", response->absolutePath);
    printf("\tHeaders:\n");
    struct Header *header = response->headers;
    while (header != NULL) {
        printf("\t\t%s: %s\n", header->name, header->value);
        header = header->next;
    }
    printf("\n\n");
}

void freeResponse(struct Response *response) {
    free(response->absolutePath);
    freeHeader(response->headers);
    free(response);
}

void sendResponse(struct Response *response, int clientFd) {

    if (response->bodyFd == -1) {
        char responseBuffer[1024];
        if (response->statusCode == HTTP_STATUS_NOT_FOUND) {
            // TODO: Change absolutePath to request->path
            sprintf(responseBuffer, notFoundResponseTemplate, response->absolutePath);
        } else {
            strcpy(responseBuffer, internalServerResponseTemplate);
        }
        sendAll(clientFd, responseBuffer, strlen(responseBuffer));
        return;
    }

    char statusCodeReason[33];
    strncpy(statusCodeReason, HTTP_STATUS_REASON(response->statusCode), sizeof(statusCodeReason));
    magic_t magic = magic_open(MAGIC_MIME_TYPE | MAGIC_PRESERVE_ATIME | MAGIC_SYMLINK | MAGIC_ERROR);
    // get current directory path
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        die("getcwd() error");
    }
    /**
     * Information about magic library
     *  http://cweiske.de/tagebuch/custom-magic-db.htm
     *  https://manpages.debian.org/testing/libmagic-dev/libmagic.3.en.html
     *  /usr/lib/file/magic.mgc
     *  /usr/share/misc/magic.mgc
     **/
    strcat(cwd, "/include/web.magic.mgc:/usr/share/misc/magic.mgc");
    if (magic_load(magic, cwd) != 0) {
        magic_close(magic);
        die("magic_load() error");
    }
    const char *magicMimeType = magic_descriptor(magic, response->bodyFd);
    char mimeType[150];
    strcpy(mimeType, magicMimeType);
    if (strcmp(mimeType, "text/") > 0) {
        strcat(mimeType, "; charset=UTF-8");
    }
    magic_close(magic);

    char lastModifiedDate[100];
    struct tm *tm = localtime(&response->lastModified);
    strftime(lastModifiedDate, 100, "%a, %d %b %Y %H:%M:%S GMT", tm);

    char currentDate[100];
    time_t now = time(NULL);
    tm = localtime(&now);
    strftime(currentDate, 100, "%a, %d %b %Y %H:%M:%S GMT", tm);

    char responseHeader[1024];
    // concat string with format https://stackoverflow.com/a/2674333
    sprintf(responseHeader, "%s %u %s\n", response->protocolVersion, response->statusCode, statusCodeReason);
    struct Header *header = response->headers;
    while (header != NULL) {
        sprintf(responseHeader + strlen(responseHeader), "%s: %s\n", header->name, header->value);
        header = header->next;
    }
    sprintf(responseHeader + strlen(responseHeader), "content-length: %lu\n", response->size);
    sprintf(responseHeader + strlen(responseHeader), "content-type: %s\n", mimeType);
    sprintf(responseHeader + strlen(responseHeader), "date: %s\n", currentDate);
    sprintf(responseHeader + strlen(responseHeader), "last-modified: %s\n", lastModifiedDate);
    sprintf(responseHeader + strlen(responseHeader), "server: %s\n", "Undefined Behaviour Server");
    sprintf(responseHeader + strlen(responseHeader), "cache-control: %s\n\n", "private, max-age=86400, must-revalidate, stale-if-error=86400");
    sendAll(clientFd, responseHeader, strlen(responseHeader));

    off_t offset = 0;
    ssize_t sendBodyLen = sendfile(clientFd, response->bodyFd, &offset, response->size);
    if (sendBodyLen == -1) {
        logError("sendfile() failed: %s", response->absolutePath);
    }
    close(response->bodyFd);
}

struct Response *makeResponse(struct Request *request, char *htmlDir) {
    struct Response *response = malloc(sizeof(struct Response));
    if (response == NULL) {
        die("malloc() response failed");
    }

    memset(response, 0, sizeof(struct Response));

    // TODO: Separate version from protocol
    response->protocolVersion = request->protocolVersion;

    char path[1024];
    bool isIndex = false;
    strcpy(path, request->path);
    char *tmpQuery = strchr(path, '?');
    if (tmpQuery != NULL) {
        *tmpQuery = '\0';
    }

    if (path[0] == '/' && path[1] == '\0') {
        isIndex = true;
        strcat(path, "index.html");
    }

    char *absolutePath = malloc(strlen(htmlDir) + strlen(path) + 1);
    sprintf(absolutePath, "%s%s", htmlDir, path);
    response->absolutePath = absolutePath;

    int bodyFd = open(response->absolutePath, O_RDONLY);
    if (bodyFd == -1) {
        char errorPath[strlen(htmlDir) + strlen("/error/error.html") + 1];
        // TODO: switch for errno with HTTP_STATUS_CODE and save message in Response ?
        if (errno == ENOENT) {
            response->statusCode = HTTP_STATUS_NOT_FOUND;
            sprintf(errorPath, "%s%s", htmlDir, "/error/404.html");
        } else {
            response->statusCode = HTTP_STATUS_INTERNAL_SERVER_ERROR;
            sprintf(errorPath, "%s%s", htmlDir, "/error/error.html");
        }
        logError("HTML file not found %s", request->path);
        bodyFd = open(errorPath, O_RDONLY);
        if (bodyFd == -1) {
            response->bodyFd = -1;
            logError("Error template not found %s", errorPath);
        } else {
            struct stat stat_buf;
            fstat(bodyFd, &stat_buf);
            response->size = stat_buf.st_size;
            response->lastModified = stat_buf.st_mtime;
            response->bodyFd = bodyFd;
        }

    } else {
        /* Stat the input file to obtain its size. */
        struct stat stat_buf;
        fstat(bodyFd, &stat_buf);
        response->lastModified = stat_buf.st_mtime;
        response->size = stat_buf.st_size;
        response->bodyFd = bodyFd;
        response->statusCode = HTTP_STATUS_OK;
    }

    return response;
}

size_t sendAll(int fd, const void *buffer, size_t count) {
    size_t left_to_write = count;
    while (left_to_write > 0) {
        size_t written = send(fd, buffer, count, 0);
        if (written == -1) {
            // An error occurred
            if (errno == EINTR) {
                // The call was interrupted by a signal, try again
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

void unsupportedProtocolResponse(int clientFd, char *protocolVersion) {
    char responseBuffer[1024];
    snprintf(responseBuffer, 1024, versionNotSupportedResponseTemplate, protocolVersion);
    sendAll(clientFd, responseBuffer, strlen(responseBuffer));
}

void helloResponse(int clientFd) {
    sendAll(clientFd, helloResponseTemplate, strlen(helloResponseTemplate));
}