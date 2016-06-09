#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "util.h"

int is_char(char c) {
	return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '_' || c == '.';
}

int is_whitespace(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

int get_value(char c) {
	if ( c >= '0' && c <= '9' ) {
		return c - '0';
	} else if ( c >= 'a' && c <= 'f' ) {
		return 0x0a + c - 'a';
	} else if ( c >= 'A' && c <= 'F' ) {
		return 0x0a + c - 'A';
	}
	return -1;
}

void current_time(char *s) {
	time_t t;
	time(&t);
	struct tm *tinfo = localtime(&t);
	strftime(s, 20, "%T %F", tinfo);
	return ;
}

int strcmp_i(char *a, char *b) {
	while ( 1 ) {
		int r = tolower(*a) - tolower(*b);
		if ( r != 0 || *a == '\0' ) {
			return r;
		}
		a++;
		b++;
	}
}