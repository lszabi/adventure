#ifndef CONSOLE_H
#define CONSOLE_H

/*

Curses-like cross-platform console library for win32 and linux
created by L Szabi 2013

version 1.2

*/

#include <stdarg.h>

// Special keys
#define CH_ESC 1
#define CH_HOME 2
#define CH_UP 3
#define CH_PAGEUP 4
#define CH_LEFT 5
#define CH_RIGHT 6
#define CH_END 7
#define CH_DOWN 8
#define CH_PAGEDOWN 9
#define CH_INSERT 10
#define CH_DEL 11
#define CH_BACKSPACE 12

// Colors
#define CL_FRONT_BLACK "\x1b[30m"
#define CL_FRONT_RED "\x1b[31m"
#define CL_FRONT_GREEN "\x1b[32m"
#define CL_FRONT_YELLOW "\x1b[33m"
#define CL_FRONT_BLUE "\x1b[34m"
#define CL_FRONT_MAGENTA "\x1b[35m"
#define CL_FRONT_CYAN "\x1b[36m"
#define CL_FRONT_WHITE "\x1b[37m"
#define CL_FRONT_DEFAULT "\x1b[39m"

#define CL_BACK_BLACK "\x1b[40m"
#define CL_BACK_RED "\x1b[41m"
#define CL_BACK_GREEN "\x1b[42m"
#define CL_BACK_YELLOW "\x1b[43m"
#define CL_BACK_BLUE "\x1b[44m"
#define CL_BACK_MAGENTA "\x1b[45m"
#define CL_BACK_CYAN "\x1b[46m"
#define CL_BACK_WHITE "\x1b[47m"
#define CL_BACK_DEFAULT "\x1b[49m"

#define CL_BOLD "\x1b[1m"
#define CL_RESET "\x1b[0m"

void setup_console();
void cleanup_console();

int get_console_width();
int read_console();
void flush_console();
char get_char();

void set_input(int);
void get_input(char *);
void get_password(char *);

char *end_input();
char *end_input_echo();

/*
Special printf function for the console driver
Use !R to align right, and use !C to align to center.
In this case, print one line only and place the '\n' to the end of the string.
*/
void cprintf(char *, ...);
void vcprintf(char *, va_list);

#endif // CONSOLE_H
