#include "common.h"

// Adventure game by L Szabi 2013

// Convert string array to int ( base 10 )
int tosint(char *c, int *err) {
	int i = 0, ret = 0, neg = 1;
	*err = 0;
	if ( c[i] == '-' ) {
		neg = -1;
		i++;
	}
	while ( c[i] ) {
		if ( c[i] >= '0' && c[i] <= '9' ) {
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
	if ( err ) {
		return -1;
	}
	return ret;
}

// Print integer and short to char array
// we are using little endian
void print_int(unsigned char *arr, int num) {
	arr[0] = num & 0xFF;
	arr[1] = ( num & 0xFF00 ) >> 8;
	arr[2] = ( num & 0xFF0000 ) >> 16;
	arr[3] = ( num & 0xFF000000 ) >> 24;
}

void print_short(unsigned char *arr, short num) {
	arr[0] = num & 0xFF;
	arr[1] = ( num & 0xFF00 ) >> 8;
}

// Get integer and short from char array
// we are using little endian
int get_int(unsigned char *arr) {
	return ( arr[3] << 24 ) | ( arr[2] << 16 ) | ( arr[1] << 8 ) | arr[0];
}

short get_short(unsigned char *arr) {
	return ( arr[1] << 8 ) | arr[0];
}

// Get proper user input
int get_input(char *message, char *buf) {
	return get_input_l(message, buf, AD_BUF);
}

int get_input_l(char *message, char *buf, int maxlen) {
	int i = 0;
	while ( !i ) {
		printf("%s", message);
		fgets(buf, maxlen, stdin);
		i = strlen(buf);
	}
	buf[--i] = '\0';
	return i;
}

// Generate a random number in range
int random(int min, int max) {
	return ( rand() + min ) % max;
}
 // Display time friendly
 char *disp_time(char *buf, int time) {
	int d = time / ( 24 * 60 * 60 );
	time %= 24 * 60 * 60;
	int h = time / ( 60 * 60 );
	time %= 60 * 60;
	int m = time / 60;
	time %= 60;
	sprintf(buf, "%i d %i h %i min %i sec", d, h, m, time);
	return buf;
}
 
// Convert ';' to '\0'
int convert_sep(char *arr) {
	int i = 0;
	while ( arr[i] != '\0' && arr[i] != ';' ) {
		i++;
	}
	arr[i] = '\0';
	return i;
}
