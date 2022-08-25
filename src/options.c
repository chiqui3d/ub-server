#include <unistd.h>

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
    "Logger path: %s\n\n",
    options.address,
    options.port,
    options.htmlDir,
    options.logger.active ? "file" : "stderr",
    options.logger.path);
}

struct Options getOptions(int argc, char *argv[]) {

    int command;
    int option_index = 0;

    struct Logger logger;
    memset(&logger, 0, sizeof(struct Logger));

    // DEFAULT LOGGER
    logger.active = false;
    logger.fileFd = -1;

    // DEFAULT OPTIONS
    struct Options options;
    memset(&options, 0, sizeof(struct Options));
    strcpy(options.address, "localhost");
    options.port = 3001;
    options.logger = logger;

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
                options.logger.active = true;
                strcpy(options.logger.path, DEFAULT_LOGGER_FILE_PATH);
                char loggerFileName[LOGGER_FILE_NAME_MAX];
                strcpy(options.logger.fileName, getLoggerFileName(loggerFileName));
                LOGGER = options.logger;
                break;
            }
            case 't': {
                strcpy(options.logger.path, optarg);
                char loggerUserFileName[LOGGER_FILE_NAME_MAX];
                strcpy(options.logger.fileName, getLoggerFileName(loggerUserFileName));
                LOGGER = options.logger;
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
    
    LOGGER = options.logger;
    OPTIONS = options;

    return options;
}