#include <stdio.h>  // for fprintf()
#include <stdlib.h> // for exit()

#include "options.h"
#include "server.h"

const char *programName;

void sigint_handler(int s) { sigintReceived = 1; }

int main(int argc, char *argv[]) {

    programName = argv[0];

    struct Options options = getOptions(argc, argv);
    
    printOptions(options);

    struct sigaction action = {};
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &action, NULL) == -1) { // SIGINT: Ctrl + c signal
        fprintf(stderr, "sigaction");
    }
    // SIGPIPE: Broken pipe with send or sendfile in response.c
    // https://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly
    signal(SIGPIPE, SIG_IGN);

    sigintReceived = 0;

    serverRun(options);

    return EXIT_SUCCESS;
}