#include "console.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
Console lib by L Szabi 2013
version 1.2
*/

#define BUFSIZE 2048
#define TAB_EN 1

#ifndef TAB_EN
#define TAB_EN 0
#endif

#ifdef WIN32
#include <windows.h>

static WORD def_attr;

void setup_console() {
	setbuf(stdout, NULL);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	def_attr = csbi.wAttributes;
}

void cleanup_console() {
}

// Read one char directly from the keyboard
static int readchar() {
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	INPUT_RECORD irInputRecord;
	DWORD dwEventsRead;
	if ( ReadConsoleInputA(hStdin, &irInputRecord, 1, &dwEventsRead)) { /* Read key press */
		if ( irInputRecord.EventType == KEY_EVENT && irInputRecord.Event.KeyEvent.bKeyDown ) {
			WORD key = irInputRecord.Event.KeyEvent.wVirtualKeyCode;
			if ( key == 0x08 ) {
				return 0x100 | CH_BACKSPACE;
			} else if ( key == 0x1B ) {
				return 0x100 | CH_ESC;
			} else if ( key == 0x24 ) {
				return 0x100 | CH_HOME;
			} else if ( key == 0x26 ) {
				return 0x100 | CH_UP;
			} else if ( key == 0x21 ) {
				return 0x100 | CH_PAGEUP;
			} else if ( key == 0x25 ) {
				return 0x100 | CH_LEFT;
			} else if ( key == 0x27 ) {
				return 0x100 | CH_RIGHT;
			} else if ( key == 0x23 ) {
				return 0x100 | CH_END;
			} else if ( key == 0x28 ) {
				return 0x100 | CH_DOWN;
			} else if ( key == 0x22 ) {
				return 0x100 | CH_PAGEDOWN;
			} else if ( key == 0x2D ) {
				return 0x100 | CH_INSERT;
			} else if ( key == 0x2E ) {
				return 0x100 | CH_DEL;
			}
			return irInputRecord.Event.KeyEvent.uChar.AsciiChar;
		}
	}
	return 0;
}

int get_console_width() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	return csbi.dwSize.X;
}

#else // not win32

#include <termios.h>
#include <sys/ioctl.h>

static struct termios default_settings;

// Set console mode
void setup_console() {
	setbuf(stdout, NULL);
	struct termios settings;
	tcgetattr(STDIN_FILENO, &settings);
	memcpy(&default_settings, &settings, sizeof(struct termios));
	settings.c_lflag &= ~( ECHO | ECHONL | ICANON | CSIZE );
	settings.c_cflag |= CS8;
	settings.c_cc[VMIN] = 0;
	settings.c_cc[VTIME] = 1;
	tcsetattr(STDIN_FILENO, TCSANOW, &settings);
}

void cleanup_console() {
	tcsetattr(STDIN_FILENO, TCSANOW, &default_settings);
}

// Read one char directly from the keyboard
static int readchar() {
	int in = getchar();
	if ( in != EOF ) {
		char c = (char)in;
		if ( c == 0x7F ) {
			return 0x100 | CH_BACKSPACE;
		}
		if ( c == 0x1B ) {
			in = getchar(); // 0x5B ( [ ) or 0x4F ( O )
			if ( in == EOF ) {
				return 0x100 | CH_ESC;
			}
			in = getchar();
			if ( in != EOF ) {
				c = (char)in;
				if ( c == 0x48 ) {
					c = CH_HOME;
					getchar(); // 0x7E
				} else if ( c == 0x41 ) {
					c = CH_UP;
				} else if ( c == 0x35 ) {
					c = CH_PAGEUP;
					getchar(); // 0x7E
				} else if ( c == 0x44 ) {
					c = CH_LEFT;
				} else if ( c == 0x43 ) {
					c = CH_RIGHT;
				} else if ( c == 0x46 ) {
					c = CH_END;
				} else if ( c == 0x42 ) {
					c = CH_DOWN;
				} else if ( c == 0x36 ) {
					c = CH_PAGEDOWN;
					getchar(); // 0x7E
				} else if ( c == 0x32 ) {
					c = CH_INSERT;
					getchar(); // 0x7E
				} else if ( c == 0x33 ) {
					c = CH_DEL;
					getchar(); // 0x7E
				} else {
					return 0;
				}
				return 0x100 | c;
			}
		}
		return c;
	}
	return 0;
}

int get_console_width() {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	return w.ws_col;
}

#endif // WIN32

// Ring buffer
static char buffer[BUFSIZE];
static char prompt[32];
static int bufp = 0; // end of current input ('\0')
static int buf_begin = 0; // first char of current input
static int buf_prev = 0; // first char of previous input
static int buf_cursor = 0; // cursor place from the end
static int show_input = 0, enable_input = 1, password_mode = 0;

// Clear the current line in the console
static void clear_line() {
	printf("\r");
	int i;
	int f = get_console_width();
#ifdef WIN32
	f--;
#endif
	for ( i = 0; i < f; i++ ) {
		printf(" ");
	}
	printf("\r");
}

// Show console input line
static void show_console() {
	if ( show_input ) {
		clear_line();
		printf("%s", prompt);
		int w = get_console_width() - 1 - strlen(prompt), buflen = ( bufp - buf_begin + BUFSIZE ) % BUFSIZE;
		int cur_pos = buflen - buf_cursor, h = w / 2, i = 0, disp_end = buflen - 1, cont = 0, cur_back = buf_cursor;
		if ( buflen >= w ) {
			if ( cur_pos <= h) { // beginning
				disp_end = w - 4;
				cont = 1;
				cur_back = w - cur_pos;
			} else if ( disp_end - cur_pos <= h ) { // end
				i = disp_end - w + 4;
				printf("...");
				cur_back = disp_end - cur_pos + 1;
			} else { // middle
				i = cur_pos - h + 3;
				printf("...");
				disp_end = cur_pos + h - 3;
				cont = 1;
				cur_back = h + 1;
			}
		}
		while ( i <= disp_end ) {
			char c = buffer[(buf_begin + i++) % BUFSIZE];
			if ( password_mode ) {
				putchar('*');
			} else {
				if ( c == '\t' ) {
					putchar('~');
				} else {
					putchar(c);
				}
			}
		}
		if ( cont ) {
			printf("...");
		}
		for ( i = 0; i < cur_back; i++ ) {
			putchar('\b');
		}
	}
}

// Add character to the ring buffer
static int add_char(char c) {
	if ( ( bufp + 2 ) % BUFSIZE != buf_prev ) {
		if ( show_input ) {
			int i = bufp;
			int to = ( bufp - buf_cursor + BUFSIZE ) % BUFSIZE;
			while ( i != to ) {
				int f = ( i - 1 + BUFSIZE ) % BUFSIZE;
				buffer[i] = buffer[f];
				i = f;
			}
			buffer[to] = c;
			bufp++;
		} else {
			buffer[bufp++] = c;
		}
		bufp %= BUFSIZE;
		buffer[bufp] = '\0';
		return 1;
	}
	return 0;
}

// Get the selected character from the ring buffer
char get_char() {
	if ( bufp != buf_begin ) {
		int buflen = ( bufp - buf_begin + BUFSIZE ) % BUFSIZE;
		bufp = ( bufp - 1 + BUFSIZE ) % BUFSIZE;
		if ( buf_cursor == buflen ) {
			buf_cursor--;
		}
		int i = ( bufp - buf_cursor + BUFSIZE ) % BUFSIZE;
		char c = buffer[i];
		while ( i != bufp ) {
			int f = ( i + 1 ) % BUFSIZE;
			buffer[i] = buffer[f];
			i = f;
		}
		buffer[i] = '\0';
		return c;
	}
	return '\0';
}

// Handle keystrokes
int read_console() { 
	/* Return:
	0 - No char
	1 - Normal char
	2 - Enter, should be handled by end_input()
	0x1xx - Special char [xx]: should be handled
	*/
	int c = readchar();
	if ( c != 0 ) {
		if ( !enable_input ) {
			return 0;
		}
		if ( c == '\n' || c == '\r' ) {
			buf_prev = buf_begin;
			buf_cursor = 0;
			add_char('\0');
			buf_begin = bufp;
			return 2;
		} else if ( !TAB_EN && c == '\t' ) {
			return 0;
		} else if ( c & 0x100 ) {
			if ( show_input ) {
				c &= 0xFF;
				int buflen = ( bufp - buf_begin + BUFSIZE ) % BUFSIZE;
				if ( c == CH_LEFT ) {
					if ( buf_cursor < buflen ) {
						buf_cursor++;
					}
				} else if ( c == CH_RIGHT ) {
					if ( buf_cursor > 0 ) {
						buf_cursor--;
					}
				} else if ( c == CH_HOME ) {
					buf_cursor = buflen;
				} else if ( c == CH_END ) {
					buf_cursor = 0;
				} else if ( c == CH_DEL ) {
					if ( buf_cursor > 0 ) {
						buf_cursor--;
					}
					if ( get_char() == '\0' ) {
						return 0;
					}
				} else if ( c == CH_BACKSPACE ) {
					if ( get_char() == '\0' ) {
						return 0;
					}
				} else {
					return 0x100 | c;
				}
			} else {
				return c;
			}
		} else {
			if ( !add_char(c) ) {
				return 0;
			}
		}
	} else {
		return 0;
	}
	show_console();
	return 1;
}

// Empty the current buffer
void flush_console() {
	bufp = buf_begin;
	buffer[bufp] = '\0';
}

// Enable key recording
void set_input(int enable) {
	enable_input = ( enable ? 1 : 0 );
}

// Start input
void get_input(char* _p) {
	flush_console();
	strncpy(prompt, _p, 31);
	enable_input = 1;
	show_input = 1;
	show_console();
}

// Start password input
void get_password(char *_p) {
	password_mode = 1;
	get_input(_p);
}

// End input session and get the result
static char *_end_input(int echo) {
	if ( show_input ) {
		password_mode = 0;
		show_input = 0;
		if ( echo ) {
			printf("\n");
		} else {
			clear_line();
		}
		return buffer + buf_prev;
	}
	return NULL;
}

char *end_input() {
	return _end_input(0);
}

char *end_input_echo() {
	return _end_input(1);
}

// Compatible printf function
#ifdef WIN32

static void color_reset() {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), def_attr);
}

static void colored_print(char *str) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hstdout, &csbi);
	WORD attributes = csbi.wAttributes;
	while ( str ) {
		char *next = strstr(str, "\x1b[");
		if ( next ) {
			*next = '\0';
			printf("%s", str);
			next += 2;
			if ( *next == '0' ) {
				attributes = def_attr;
			} else if ( *next == '1' ) {
				attributes |= FOREGROUND_INTENSITY;
			} else if ( *next == '3' ) { // text
				next++;
				attributes &= ~( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );
				char c = *next;
				if ( c == '1' || c == '3' || c == '5' || c == '7' ) {
					attributes |= FOREGROUND_RED;
				}
				if ( c == '2' || c == '3' || c == '6' || c == '7' ) {
					attributes |= FOREGROUND_GREEN;
				}
				if ( c == '4' || c == '5' || c == '6' || c == '7' ) {
					attributes |= FOREGROUND_BLUE;
				}
				if ( c == '9' ) {
					attributes |= def_attr & ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );
				}
			} else if ( *next == '4' ) { // background
				next++;
				attributes &= ~( BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE );
				char c = *next;
				if ( c == '1' || c == '3' || c == '5' || c == '7' ) {
					attributes |= BACKGROUND_RED;
				}
				if ( c == '2' || c == '3' || c == '6' || c == '7' ) {
					attributes |= BACKGROUND_GREEN;
				}
				if ( c == '4' || c == '5' || c == '6' || c == '7' ) {
					attributes |= BACKGROUND_BLUE;
				}
				if ( c == '9' ) {
					attributes |= def_attr & ( BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE );
				}
			}
			SetConsoleTextAttribute(hstdout, attributes);
			str = next + 2;
		} else {
			printf("%s", str);
			break;
		}
	}
}

#else

static void color_reset() {
	printf("%s", CL_RESET);
}

static void colored_print(char *str) {
	printf("%s", str);
}

#endif

static int colored_strlen(char *str) {
	int len = 0, skip = 0;
	while ( *str ) {
		if ( *str == '\x1b' ) {
			skip = 1;
		}
		if ( !skip ) {
			len++;
		}
		if ( skip && *str == 'm' ) {
			skip = 0;
		}
		str++;
	}
	return len;
}

void cprintf(char *format, ...) {
	va_list args;
	va_start(args, format);
	vcprintf(format, args);
	va_end(args);
}
	
void vcprintf(char *format, va_list args) {
	char buf[BUFSIZE];
	clear_line();
	vsprintf(buf, format, args);
	char *center = strstr(buf, "!C");
	char *right = strstr(buf, "!R");
	if ( center ) {
		*center = '\0';
		center += 2;
	}
	if ( right ) {
		*right = '\0';
		right += 2;
	}
	int pos = colored_strlen(buf);
	colored_print(buf);
	if ( center ) {
		int clen = colored_strlen(center);
		int len = ( get_console_width() - clen ) / 2 - pos;
		printf("%*s", len, " ");
		colored_print(center);
		pos += len + clen;
	}
	if ( right ) {
		int len = get_console_width() - pos - colored_strlen(right);
		int nl = 0;
		if ( right[strlen(right) - 1] == '\n' ) {
			right[strlen(right) - 1] = '\0';
			len += 1;
			nl = 1;
		} else {
			len -= 1; // leave a space at the end of the line
		}
		printf("%*s", len, " ");
		colored_print(right);
#ifndef WIN32
		if ( nl ) {
			printf("\n");
		}
#endif
	}
	color_reset();
	show_console();
}
