#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>

#include "die.h"

void die(const char *message, ...) {

  va_list args;
  va_start(args, message);
  char buffer[CHAR_MAX];
  vsprintf(buffer, message, args);
  va_end(args);

  if (errno != 0) {
    fprintf(stderr, "%s | %s", buffer, strerror(errno));
  } else {
    fprintf(stderr, "%s", buffer);
  }
  fprintf(stderr, "\n");

  exit(EXIT_FAILURE);
}