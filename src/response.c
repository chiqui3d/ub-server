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
#include "helper.h"
#include "response.h"
#include "server.h"

void unsupportedProtocolResponse(int clientFd, char *protocolVersion) {
    char responseBuffer[1024];
    snprintf(responseBuffer, 1024, versionNotSupportedResponseTemplate, protocolVersion);
    sendAll(clientFd, responseBuffer, strlen(responseBuffer));
}

void tooManyRequestResponse(int clientFd) {
    sendAll(clientFd, tooManyRequestResponseTemplate, strlen(tooManyRequestResponseTemplate));
}

void helloResponse(int clientFd) {
    sendAll(clientFd, helloResponseTemplate, strlen(helloResponseTemplate));
}

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

struct Response *makeResponse(struct Request *request, char *htmlDir) {
    struct Response *response = malloc(sizeof(struct Response));
    if (response == NULL) {
        die("malloc() response failed");
    }

    memset(response, 0, sizeof(struct Response));

    // TODO: Separate version from protocol
    response->protocolVersion = request->protocolVersion;

    // get connection header
    char *connectionHeader = getHeader(request->headers, "connection");

    // If there is no connection header or the header is equal to close, then add close connection header
    if (connectionHeader == NULL || (connectionHeader != NULL && *connectionHeader == 'c')) {
        response->closeConnection = true;
        response->headers = addHeader(response->headers, "connection", "close");
    }

    // add keep-alive header
    if (connectionHeader != NULL && *connectionHeader == 'k') {
        response->closeConnection = false;

        size_t lenHeaderKeepAlive = snprintf(NULL, 0, "timeout=%i", KEEP_ALIVE_TIMEOUT);
        char headerKeepAliveValue[lenHeaderKeepAlive + 1];
        snprintf(headerKeepAliveValue, lenHeaderKeepAlive + 1, "timeout=%i", KEEP_ALIVE_TIMEOUT);  

        response->headers = addHeader(response->headers, "connection", "keep-alive");                  
        response->headers = addHeader(response->headers, "keep-alive", headerKeepAliveValue);
    }

    char path[1024];
    bool isIndex = false;
    strCopySafe(path, request->path);
    char *tmpQuery = strchr(path, '?');
    if (tmpQuery != NULL) {
        *tmpQuery = '\0';
    }

    if (path[0] == '/' && path[1] == '\0') {
        isIndex = true;
        strncat(path, "index.html", 11);
    }
    size_t absolutePathSize = strlen(htmlDir) + strlen(path) + 1;
    char absolutePath[absolutePathSize];
    snprintf(absolutePath, absolutePathSize, "%s%s", htmlDir, path);
    response->absolutePath = strdup(absolutePath);

    int bodyFd = open(response->absolutePath, O_RDONLY);
    if (bodyFd == -1) {
        size_t errorPathSize = strlen(htmlDir) + strlen("/error/error.html") + 1;
        char errorPath[errorPathSize];
        // TODO: switch for errno with HTTP_STATUS_CODE and save message in Response ?
        if (errno == ENOENT) {
            response->statusCode = HTTP_STATUS_NOT_FOUND;
            snprintf(errorPath, errorPathSize, "%s%s", htmlDir, "/error/404.html");
        } else {
            response->statusCode = HTTP_STATUS_INTERNAL_SERVER_ERROR;
            snprintf(errorPath, errorPathSize, "%s%s", htmlDir, "/error/error.html");
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

void sendResponse(struct Response *response, struct Request *request, int clientFd) {

    if (response->bodyFd == -1) {
        char responseBuffer[1024];
        if (response->statusCode == HTTP_STATUS_NOT_FOUND) {
            snprintf(responseBuffer, sizeof(responseBuffer), notFoundResponseTemplate, request->path);
        } else {
            snprintf(responseBuffer, sizeof(responseBuffer), "%s", internalServerResponseTemplate);
        }
        sendAll(clientFd, responseBuffer, strlen(responseBuffer));
        return;
    }

    char statusCodeReason[33];
    strCopySafe(statusCodeReason, (char*)HTTP_STATUS_REASON(response->statusCode));

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
    strncat(cwd, "/include/web.magic.mgc:/usr/share/misc/magic.mgc", 49);
    if (magic_load(magic, cwd) != 0) {
        magic_close(magic);
        die("magic_load() error");
    }
    const char *magicMimeType = magic_descriptor(magic, response->bodyFd);
    char mimeType[150];
    strCopySafe(mimeType, (char *)magicMimeType);

    if (strcmp(mimeType, "text/") > 0) {
        strncat(mimeType, "; charset=UTF-8", 16);
    }
    magic_close(magic);

    char lastModifiedDate[100];
    struct tm *tm = localtime(&response->lastModified);
    strftime(lastModifiedDate, 100, "%a, %d %b %Y %H:%M:%S GMT", tm);

    char currentDate[100];
    time_t now = time(NULL);
    tm = localtime(&now);
    strftime(currentDate, 100, "%a, %d %b %Y %H:%M:%S GMT", tm);

    size_t responseHeaderSize = 1024;
    char responseHeader[responseHeaderSize];
    size_t offset = snprintf(responseHeader, responseHeaderSize, "%s %u %s\n", response->protocolVersion, response->statusCode, statusCodeReason);
    struct Header *header = response->headers;
    while (header != NULL) {
        offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "%s: %s\n", header->name, header->value);
        header = header->next;
    }

    offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "content-length: %lu\n", response->size);
    offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "content-type: %s\n", mimeType);
    offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "date: %s\n", currentDate);
    offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "last-modified: %s\n", lastModifiedDate);
    offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "server: %s\n", "Undefined Behaviour Server");
    // sprintf(responseHeader + strlen(responseHeader), "cache-control: %s\n\n", "private, max-age=86400, must-revalidate, stale-if-error=86400");
    offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "cache-control: %s\n\n", "private, no-cache, no-store, must-revalidate");

    sendAll(clientFd, responseHeader, offset);

    off_t bodyOffset = 0;
    ssize_t sendBodyLen = sendfile(clientFd, response->bodyFd, &bodyOffset, response->size);
    if (sendBodyLen == -1) {
        logError("sendfile() failed: %s", response->absolutePath);
    }
    close(response->bodyFd);
}

size_t sendAll(int fd, const void *buffer, size_t count) {
    size_t left_to_write = count;
    while (left_to_write > 0) {
        size_t written = send(fd, buffer, count, 0);
        if (written == -1) {
            if (errno == EINTR) {
                // The send call was interrupted by a signal
                return 0;
            }
            return -1;
        } else {
            left_to_write -= written;
        }
    }

    return count;
}
