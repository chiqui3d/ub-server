#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdio.h>  // for fprintf()
#include <stdlib.h> // for exit()
#include <getopt.h> // for struct option and getopt_long()
#include <stdint.h> // for uint16_t
#include <stdbool.h> // for bool
#include <string.h> // for strcpy()
#include <ctype.h> // for isprint()
#include <arpa/inet.h> // for INET6_ADDRSTRLEN


#include "../lib/logger/logger.h" // for struct Logger

#define OPTIONS_PATH_MAX 4096


static const char *usageTemplate =
    "Usage: %s [ options ]\n\n"
    "  -a, --address ADDR        Bind to local address (by default localhost)\n"
    "  -p, --port PORT           Bind to specified port (by default, 3001)\n"
    "  -d, --html-dir DIR        Open html and assets from specified directory (by default <public> directory)\n"
    "  -l, --logger              Active logs for send messages to log file (by default logs send to stderr)\n"
    "  --logger-path             Absolute path to directory (by default /var/log/ub-server/)\n"
    "  -h, --help                Print this usage information\n";

static struct option longOptions[] = {
    {"address", required_argument, NULL, 'a'},
    {"port", required_argument, NULL, 'p'},
    {"html-dir", required_argument, NULL, 'd'},
    {"logger", no_argument, NULL, 'l'},
    {"logger-path", required_argument, NULL, 't'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0},
};

static char *const shortOptions = "a:p:d:lht:";

extern const char *programName;
static const char *usageTemplate;

struct Options {
    char address[INET6_ADDRSTRLEN]; // IPv4 or IPv6
    uint16_t port;
    char htmlDir[OPTIONS_PATH_MAX];
    bool TCPKeepAlive;
};

extern struct Options OPTIONS;

void printUsage(bool is_error);
struct Options getOptions(int argc, char *argv[]);
void printOptions(struct Options options);

#endif // OPTIONS_H