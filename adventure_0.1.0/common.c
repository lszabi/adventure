#include "common.h"

// Adventure game by L Szabi 2013

// Convert string array to int ( base 10 )
int tosint(char *c, int *err) {
	int i = 0, ret = 0, neg = 1;
	// *err = 0;
	if ( c[0] == '-' ) {
		neg = -1;
		i++;
	}
	while ( c[i] ) {
		if ( RANGE(c[i], '0', '9') ) {
			ret = ret * 10 + ( c[i++] - '0' );
		} else {
			*err = 1;
			break;
		}
	}
	ret *= neg;
	return ret;
}

int toint(char *c) {
	int err = 0;
	int ret = tosint(c, &err);
	if ( ret < 0 || err ) {
		return -1;
	}
	return ret;
}

int tohex(char *c) {
	int ret = 0;
	for ( int i = 0; c[i]; i++ ) {
		if ( RANGE(c[i], '0', '9') ) {
			ret = ret * 16 + ( c[i] - '0' );
		} else if ( RANGE(c[i], 'a', 'f') ) {
			ret = ret * 16 + ( 10 + c[i] - 'a' );
		} else if ( RANGE(c[i], 'A', 'F') ) {
			ret = ret * 16 + ( 10 + c[i] - 'A' );
		} else {
			ret = -1;
			break;
		}
	}
	return ret;
}

// Generate a random number in range
int random_range(int min, int max) {
	return ( rand() % ( max - min + 1 ) ) + min;
}

// Display time friendly
char *display_time(char *buf, int t) {
	int d = t / ( 24 * 60 * 60 );
	t %= 24 * 60 * 60;
	int h = t / ( 60 * 60 );
	t %= 60 * 60;
	int m = t / 60;
	t %= 60;
	sprintf(buf, "%i d %i h %i min %i sec", d, h, m, t);
	return buf;
}

time_t get_time() {
	time_t t;
	time(&t);
	return t;
}

char *current_time() {
	time_t t;
	time(&t);
	return ctime(&t);
}
 
// Convert delimiters to '\0', organize into array
int str_explode(char *str, char **array, char separator) {
	return str_explode_max(str, array, separator, 0);
}

int str_explode_max(char *str, char **array, char separator, int limit) {
	int i, f = 1;
	array[0] = str;
	for ( i = 0; str[i]; i++ ) {
		if ( ( limit == 0 || f < limit ) && str[i] == separator ) {
			str[i] = '\0';
			array[f++] = &str[i + 1];
		}
	}	
	return f;
}

char *str_explode_next(char *str, char separator) {
	for ( int i = 0; str[i]; i++ ) {
		if ( str[i] == separator ) {
			str[i] = '\0';
			return str + i + 1;
		}
	}
	return NULL;
}

// Are char strings equal?
int char_eq(char *a, char *b) {
	return ( strcasecmp(a, b) == 0 );
}
