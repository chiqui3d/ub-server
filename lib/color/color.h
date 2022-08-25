#ifndef COLOR_H
#define COLOR_H

// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
// https://stackoverflow.com/a/23657072
// https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c
// https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
// https://stackoverflow.com/a/3219459

#define UNDERLINE "\x1B[4;37m"
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN "\x1B[36m"
#define WHITE "\x1B[37m"
#define RESET "\x1B[0m"

#endif // COLOR_H

