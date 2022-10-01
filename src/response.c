#include "zlib.h"         // for gzopen() gzip compression"
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
#include "header.h"
#include "helper.h"
#include "options.h"
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

void badRequestResponse(int clientFd) {
    sendAll(clientFd, badRequestResponseTemplate, strlen(badRequestResponseTemplate));
}

void helloResponse(int clientFd) { sendAll(clientFd, helloResponseTemplate, strlen(helloResponseTemplate)); }

void makeResponse(struct QueueConnectionElementType *connection) {

    /******* 1. Get file fd (bodyFd) *******/
    struct stat statResponseBodyFd;
    int bodyFd = open(connection->absolutePath, O_RDONLY);
    if (bodyFd == -1) {
        logError("Absolute path file %s not found", connection->absolutePath);
        // read html template for errors
        size_t errorPathSize = strlen(OPTIONS.htmlDir) + strlen("/error/error.html") + 1;
        char errorPath[errorPathSize];
        // TODO: switch for errno with HTTP_STATUS_CODE and save message in Response ?
        if (errno == ENOENT) {
            connection->responseStatusCode = HTTP_STATUS_NOT_FOUND;
            snprintf(errorPath, errorPathSize, "%s%s", OPTIONS.htmlDir, "/error/404.html");
        } else {
            connection->responseStatusCode = HTTP_STATUS_INTERNAL_SERVER_ERROR;
            snprintf(errorPath, errorPathSize, "%s%s", OPTIONS.htmlDir, "/error/error.html");
        }
        bodyFd = open(errorPath, O_RDONLY);
        if (bodyFd == -1) {
            die("Error template not found %s", errorPath);
        }
        fstat(bodyFd, &statResponseBodyFd);

    } else {
        /* Stat the input file to obtain its size. */
        fstat(bodyFd, &statResponseBodyFd);
        connection->responseStatusCode = HTTP_STATUS_OK;
    }

    connection->bodyFd = bodyFd;
    connection->bodyLength = statResponseBodyFd.st_size;
    connection->bodyOffset = 0;

    /** Get mime type **/
    char mimeType[150];
    getMimeType(connection, mimeType);

    /** Generate gzip encoding **/
    char *acceptEncodingHeader = getHeader(connection->requestHeaders, "accept-encoding");
    if (acceptEncodingHeader != NULL && strstr(acceptEncodingHeader, "gzip") != NULL) {
        makeContentEncoding(connection, statResponseBodyFd, mimeType);
    }

    /******* 2. make response headers *******/
    size_t responseHeaderSize = 1024;
    char responseHeader[responseHeaderSize];

    char statusCodeReason[33];
    strCopySafe(statusCodeReason, (char *)HTTP_STATUS_REASON(connection->responseStatusCode));

    char lastModifiedDate[100];
    struct tm *tm = localtime(&statResponseBodyFd.st_mtime);
    strftime(lastModifiedDate, 100, "%a, %d %b %Y %H:%M:%S GMT", tm);

    char currentDate[100];
    time_t now = time(NULL);
    tm = localtime(&now);
    strftime(currentDate, 100, "%a, %d %b %Y %H:%M:%S GMT", tm);

    size_t offset = snprintf(responseHeader,
                             responseHeaderSize,
                             "%s %u %s\n",
                             connection->protocolVersion,
                             connection->responseStatusCode,
                             statusCodeReason);

    // TODO: handle Content-Encoding, hardcode for now
    if (connection->contentEncoding == CONTENT_ENCODING_GZIP) {
        offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "content-encoding: gzip\n");
    }

    // get connection header request
    char *connectionHeader = getHeader(connection->requestHeaders, "connection");

    // add keep-alive header
    if (connectionHeader != NULL && *connectionHeader == 'k') {
        connection->keepAlive = true;
        size_t lenHeaderKeepAlive = snprintf(NULL, 0, "timeout=%i", KEEP_ALIVE_TIMEOUT);
        char headerKeepAliveValue[lenHeaderKeepAlive + 1];
        snprintf(headerKeepAliveValue, lenHeaderKeepAlive + 1, "timeout=%i", KEEP_ALIVE_TIMEOUT);

        offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "connection: %s\n", "keep-alive");
        offset +=
            snprintf(responseHeader + offset, responseHeaderSize - offset, "keep-alive: %s\n", headerKeepAliveValue);
    } else {

        connection->keepAlive = false;
        offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "connection: close\n");
    }

    offset +=
        snprintf(responseHeader + offset, responseHeaderSize - offset, "content-length: %lu\n", connection->bodyLength);
    offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "content-type: %s\n", mimeType);
    offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "date: %s\n", currentDate);
    offset += snprintf(responseHeader + offset, responseHeaderSize - offset, "last-modified: %s\n", lastModifiedDate);
    offset +=
        snprintf(responseHeader + offset, responseHeaderSize - offset, "server: %s\n", "Undefined Behaviour Server");
    // sprintf(responseHeader + strlen(responseHeader), "cache-control: %s\n\n", "private, max-age=86400,
    // must-revalidate, stale-if-error=86400");
    offset += snprintf(responseHeader + offset,
                       responseHeaderSize - offset,
                       "cache-control: %s\n\n",
                       "private, no-cache, no-store, must-revalidate");

    // save headers in the buffer
    connection->responseBufferHeadersOffset = 0;
    connection->responseBufferHeadersLength = offset;
    connection->responseBufferHeaders = strdup(responseHeader);
}

void sendResponseHeaders(struct QueueConnectionElementType *connection) {
    while (1) {
        ssize_t bytesSend = send(connection->clientFd,
                                 connection->responseBufferHeaders + connection->responseBufferHeadersOffset,
                                 connection->responseBufferHeadersLength - connection->responseBufferHeadersOffset,
                                 0);
        if (bytesSend < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (connection->responseBufferHeadersOffset == connection->responseBufferHeadersLength) {
                    connection->state = STATE_CONNECTION_SEND_BODY;
                    connection->responseBufferHeadersOffset = 0;
                    return;
                }
                logDebug("sendResponseHeaders EWOULDBLOCK|EAGAIN");
                return;
            }
            logError("send() response failed. DoneForClose");
            connection->state = STATE_CONNECTION_DONE_FOR_CLOSE;
            return;
        }
        connection->responseBufferHeadersOffset += bytesSend;
        if (bytesSend == 0) {
            logDebug("0 bytes send, client disconnected");
            connection->state = STATE_CONNECTION_DONE_FOR_CLOSE;
            return;
        }
        if (connection->responseBufferHeadersOffset == connection->responseBufferHeadersLength) {
            connection->state = STATE_CONNECTION_SEND_BODY;
            connection->responseBufferHeadersOffset = 0;
            return;
        }
    }
}

void sendResponseFile(struct QueueConnectionElementType *connection) {

    if (connection->bodyFd == -1) {
        connection->state = STATE_CONNECTION_DONE;
        connection->bodyOffset = 0;
        return;
    }
    while (1) {
        ssize_t bytesSend = sendfile(connection->clientFd,
                                     connection->bodyFd,
                                     &connection->bodyOffset,
                                     connection->bodyLength - connection->bodyOffset);
        if (bytesSend < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (connection->bodyOffset == connection->bodyLength) {
                    connection->state = STATE_CONNECTION_DONE;
                    connection->bodyOffset = 0;
                    close(connection->bodyFd);
                    connection->bodyFd = -1;
                    return;
                }
                logDebug("sendResponseFile EWOULDBLOCK|EAGAIN");
                return;
            }
            logError("sendfile() response failed. DoneForClose");
            connection->state = STATE_CONNECTION_DONE_FOR_CLOSE;
            return;
        }
        if (bytesSend == 0) {
            logDebug("0 bytes send with sendfile, client disconnected");
            connection->state = STATE_CONNECTION_DONE_FOR_CLOSE;
            return;
        }
        if (connection->bodyOffset == connection->bodyLength) {
            connection->state = STATE_CONNECTION_DONE;
            connection->bodyOffset = 0;
            close(connection->bodyFd);
            connection->bodyFd = -1;
            return;
        }
    }
}

void getMimeType(struct QueueConnectionElementType *connection, char *mimeType) {

    magic_t magic = magic_open(MAGIC_MIME_TYPE | MAGIC_PRESERVE_ATIME | MAGIC_SYMLINK);
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
    const char *magicMimeType = magic_descriptor(magic, connection->bodyFd);
    strCopySafe(mimeType, (char *)magicMimeType);

    if (strcmp(mimeType, "text/") > 0) {
        strncat(mimeType, "; charset=UTF-8", 16);
    }
    magic_close(magic);
}

void makeContentEncoding(struct QueueConnectionElementType *connection, struct stat statResponseBodyFd,
                         char *mimeType) {
    // get file name from absolute path
    char *fileName = strrchr(connection->absolutePath, '/');
    if (fileName == NULL) {
        // TODO: Support for directory listing or index.html
        die("File name not found in absolute path %s, possibly a directory", connection->absolutePath);
    }
    fileName++;

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        die("getcwd() error");
    }

    char relativePathFile[strlen(connection->absolutePath)];
    strCopySafe(relativePathFile, connection->absolutePath + strlen(OPTIONS.htmlDir));

    size_t relativePathLength = strlen(relativePathFile) - strlen(fileName);
    char relativePath[relativePathLength + 1];
    strncpy(relativePath, relativePathFile, relativePathLength);
    relativePath[relativePathLength] = '\0';

    size_t pathGzipCacheLen = snprintf(NULL, 0, "%s/cache/gzip%s", cwd, relativePath);
    pathGzipCacheLen++;
    char pathGzipCache[pathGzipCacheLen];

    snprintf(pathGzipCache, pathGzipCacheLen, "%s/cache/gzip%s", cwd, relativePath);
    if (makeDirectory(pathGzipCache, 755) == -1) {
        die("Error creating directory: %s", pathGzipCache);
    }

    size_t gzipPathLen = snprintf(NULL, 0, "%s%ld-%s.gz", pathGzipCache, statResponseBodyFd.st_atime, fileName);
    gzipPathLen++; // for null terminator
    char gzipPath[gzipPathLen];
    snprintf(gzipPath, gzipPathLen, "%s%ld-%s.gz", pathGzipCache, statResponseBodyFd.st_atime, fileName);

    // open gzip file
    if (access(gzipPath, F_OK) != 0) {
        logDebug("Gzip file %s not found. Create .gz file", gzipPath);
        gzFile gzFd = gzopen(gzipPath, "wb");
        if (gzFd == NULL) {
            die("Error opening gzip file %s", gzipPath);
        }
        // level 1-9, 1 is fastest, 9 is best compression
        if(gzsetparams(gzFd, 6, Z_DEFAULT_STRATEGY) != Z_OK) {
            die("Error setting gzip parameters");
        }

        // copy bodyFd to gzFd
        char bufferEncoding[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(connection->bodyFd, bufferEncoding, 1024)) > 0) {
            gzwrite(gzFd, bufferEncoding, bytesRead);
        }
        gzclose(gzFd);
    }
    close(connection->bodyFd);
    // open gzip file
    int gzipFd = open(gzipPath, O_RDONLY);
    if (gzipFd == -1) {
        die("Error opening gzip file %s", gzipPath);
    }
    struct stat statResponseGzBodyFd;
    // Stat the input file to obtain its size.
    fstat(gzipFd, &statResponseGzBodyFd);
    // set gzip header
    connection->bodyFd = gzipFd;
    connection->bodyLength = statResponseGzBodyFd.st_size;
    connection->contentEncoding = CONTENT_ENCODING_GZIP;
}