#ifndef COMMON_H
#define COMMON_H

// Adventure game by L Szabi 2013

#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
	#include <winsock2.h> // -lws2_32
	#include <windows.h>
	typedef int socklen_t;
	#define WSA_CLEAN() WSACleanup()
	#define SHUT_RDWR SD_BOTH
	#define sleep(a) Sleep(1000 * a)
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#define WSA_CLEAN()
#endif
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#define AD_PORT 25600
#define AD_BUF 256

// Commands by server
#define AD_MAGIC_SERVER 0xBD // whatever...
#define AD_CLOSE 0x10
#define AD_AUTH 0x11
#define AD_PASSW 0x12
#define AD_ACK 0x13
#define AD_VER 0x14
#define AD_MSG 0x15
#define AD_MULTI 0x16
#define AD_NOP 0x17
#define AD_BUSY 0x18
#define AD_PLIST 0x19

// Commands by player
#define AD_MAGIC_CLIENT 0xCC
#define AD_LOGIN 0x20
#define AD_LOGOUT 0x21
#define AD_OK 0x22
#define AD_REG 0x24
#define AD_CHPASSW 0x25
#define AD_ENTPASSW 0x26
#define AD_UPD 0x27
#define AD_SENDMSG 0x28
#define AD_TASK 0x29
#define AD_ABORT 0x2A
#define AD_GETPLIST 0x2B
#define AD_RCON_MSG 0x2C

// Error commands
#define AD_ERR 0x30
#define AD_ERR_MAGIC 0x31
#define AD_ERR_VER 0x32
#define AD_ERR_UNAME 0x33
#define AD_ERR_COMM 0x34
#define AD_ERR_PASSW 0x35
#define AD_ERR_BAN 0x36

// Status commands
#define AD_S_POS 0x40
#define AD_S_PROF 0x41
#define AD_S_INV 0x42

// Task commands
#define AD_T_IDLE 0x00
#define AD_T_TRAVEL 0x50

// Player states
#define AD_STATE_INIT 0
#define AD_STATE_VER 1
#define AD_STATE_AUTH 2
#define AD_STATE_LOGOUT 3
#define AD_STATE_LOGIN 4
#define AD_STATE_REG 5
#define AD_STATE_MAIN 6

// Player flag mask
#define AD_FLAG_RCON 0x01
#define AD_FLAG_BANNED 0x02

// Common functions
int toint(char *);
int tosint(char *, int *);
void print_int(unsigned char *, int);
void print_short(unsigned char *, short);
int get_int(unsigned char *);
short get_short(unsigned char *);
int get_input(char *, char *);
int get_input_l(char *, char *, int);
int random(int, int);
char *disp_time(char *, int);
int convert_sep(char *);

#endif
