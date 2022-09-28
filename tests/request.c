/**
 * @author skeeto from https://www.reddit.com/r/C_Programming
 * 
 */

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// send byte at a time, until we send the entire buffer
static int request(int fd) {
    for (char *s = "GET / HTTP/1.0\r\n\r\n"; *s; s++) {
        if (write(fd, s, 1) == -1) {
            return -1;
        }
        if (usleep(100) == -1) {
            return -1;
        }
    }
    return 0;
}

static int response(int fd) {
    char buf[1 << 12];
    for (;;) {
        int len = read(fd, buf, sizeof(buf));
        switch (len) {
            case -1:
                switch (errno) {
                    case EINTR:
                        continue;
                    default:
                        return -1;
                }
            case 0:
                return !fflush(stdout) ? -1 : 0;
            default:
                if (!fwrite(buf, len, 1, stdout)) {
                    return -1;
                }
        }
    }
}

int main(void) {
    struct addrinfo *r = 0, hints = {
                                .ai_family = AF_UNSPEC,
                                .ai_socktype = SOCK_STREAM,
                            };

    if (getaddrinfo("localhost", "3001", &hints, &r)) {
        perror("getaddrinfo()");
        return 1;
    }
    
    for (struct addrinfo *a = r; a; a = a->ai_next) {
        int socketFd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (socketFd == -1) {
            perror("socket()");
            continue;
        }

        if (connect(socketFd, a->ai_addr, a->ai_addrlen) == -1) {
            perror("connect()");
            close(socketFd);
            continue;
        }
        freeaddrinfo(r);

        if (request(socketFd) == -1) {
            perror("HTTP request");
        }
        if (response(socketFd) == -1) {
            perror("HTTP response");
        }
        close(socketFd);
        return 0;
    }
    return 1;
}