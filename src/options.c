#include <unistd.h>
#include <pthread.h>


#include "options.h"
#include "../lib/logger/logger.h"
#include "../lib/die/die.h"
#include "../lib/color/color.h"


struct Logger LOGGER;
struct Options OPTIONS;


void printUsage(bool is_error) {
    fprintf(is_error ? stderr : stdout, usageTemplate, programName);
    exit(is_error ? 1 : 0);
}

void printOptions(struct Options options) {
    consoleDebug(GREEN "Server options: \n" RESET
    "Address: %s\n"
    "Port: %d\n"
    "HTML dir: %s\n"
    "Logger: %s\n"
    "Logger path: %s\n"
    "TCP Keep-Alive: %s\n\n",
    options.address,
    options.port,
    options.htmlDir,
    LOGGER.active ? "file" : "stderr",
    LOGGER.path,
    options.TCPKeepAlive ? "On" : "Off");
}

struct Options getOptions(int argc, char *argv[]) {

    int command;
    int option_index = 0;

    // DEFAULT LOGGER
    LOGGER.active = false;
    LOGGER.fileFd = STDERR_FILENO;
    LOGGER.initialized = false;
    LOGGER.lock =  (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    // DEFAULT OPTIONS
    struct Options options;
    memset(&options, 0, sizeof(struct Options));
    strcpy(options.address, "localhost");
    options.port = 3001;
    options.TCPKeepAlive = false;


      // Default html directory
    char HtmlDir[OPTIONS_PATH_MAX];
    if (getcwd(HtmlDir, sizeof(HtmlDir)) == NULL) {
        die("HtmlDir getcwd() error");
    }
    strcat(HtmlDir, "/public");
    strcpy(options.htmlDir, HtmlDir);

    while ((command = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1) {
        switch (command) {
            case 'a':
                strcpy(options.address, optarg);
                break;
            case 'p':
                options.port = (uint16_t)atoi(optarg);
                break;
            case 'd':
                strcpy(options.htmlDir, optarg);
                break;
            case 'l': {
                LOGGER.active = true;
                strcpy(LOGGER.path, DEFAULT_LOGGER_FILE_PATH);
                char loggerFileName[LOGGER_FILE_NAME_MAX];
                strcpy(LOGGER.fileName, getLoggerFileName(loggerFileName));
                break;
            }
            case 't': {
                LOGGER.active = true;
                strcpy(LOGGER.path, optarg);
                char loggerFileName[LOGGER_FILE_NAME_MAX];
                strcpy(LOGGER.fileName, getLoggerFileName(loggerFileName));
                break;
            }

            case 'h':
                printUsage(0);
            case '?':
                if (optopt == 'a' || optopt == 'p' || optopt == 'd')
                    fprintf(stderr, "-%c, --%s requires an argument.\n", optopt, longOptions[option_index].name);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option '-%c'.\n", optopt);

                printUsage(1);
            default:
                abort();
        }
    }
    // not arguments
    /* if (optind != argc) {
        print_usage(1);
    } */
    
    OPTIONS = options;

    return options;
}