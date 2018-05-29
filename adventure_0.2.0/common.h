#ifndef COMMON_H
#define COMMON_H

/*
Adventure game by L Szabi 2013
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define RANGE(n, a, b) ( (n) >= (a) && (n) <= (b) )

#define AD_PORT 25600
#define BACKUP_TIME 3600
#define PROCESS_DELAY 1

#define AD_VER_MAJ 0
#define AD_VER_MIN 2
#define AD_VER_REV 0

// Tasks
#define AD_TASK_IDLE 0x00
#define AD_TASK_TRAVEL 0x01

// Task structures

typedef struct {
	short x;
	short y;
} __attribute__((packed)) task_travel_t;

// Packet types
#define AD_PACKET_CLOSE 0x00
#define AD_PACKET_VERSION 0x01
#define AD_PACKET_AUTH 0x02
#define AD_PACKET_ERROR 0x03
#define AD_PACKET_PLAYER 0x04
#define AD_PACKET_CMD 0x05
#define AD_PACKET_UNEXP 0x06
#define AD_PACKET_MULTI 0x07
#define AD_PACKET_TASK 0x08

// Error codes
#define AD_ERROR_OK 0x00
#define AD_ERROR_VERSION 0x01
#define AD_ERROR_PROTOCOL 0x02
#define AD_ERROR_UNKNOWN 0x03
#define AD_ERROR_REGISTER 0x04
#define AD_ERROR_LOGIN 0x05
#define AD_ERROR_PASSWD 0x06
#define AD_ERROR_SHUTDOWN 0x07
#define AD_ERROR_BANNED 0x08
#define AD_ERROR_RCON 0x09
#define AD_ERROR_SENDMSG 0x10
#define AD_ERROR_TASK 0x11
#define AD_ERROR_GIVE 0x12

// Flags
#define AD_ERROR_FLAG_MSG 0x01
#define AD_ERROR_FLAG_CLOSE 0x02

#define AD_PLAYER_FLAG_BANNED 0x01
#define AD_PLAYER_FLAG_RCON 0x02

// Client commands
#define AD_CMD_NOP 0x00
#define AD_CMD_PLAYER 0x01
#define AD_CMD_RCON 0x02
#define AD_CMD_SENDMSG 0x03
#define AD_CMD_INVERTORY 0x04
#define AD_CMD_GIVE 0x05

// Packets

typedef struct {
	uint16_t major;
	uint16_t minor;
	uint16_t revision;
} __attribute__((packed)) packet_version_t;

typedef struct {
	uint8_t new_player;
	char hash[16];
	char player_name[16];
} __attribute__((packed)) packet_auth_t;

typedef struct {
	uint16_t code;
	uint8_t flags;
	char msg[128];
} __attribute__((packed)) packet_error_t;

typedef struct {
	char name[16];
	short x, y;
	int xp, level, hp, movement_speed;
	uint32_t flags;
} __attribute__((packed)) packet_player_t;

typedef struct {
	uint16_t cmd;
	char arg[128];
	int arg_n[4];
} __attribute__((packed)) packet_cmd_t;

typedef struct {
	uint8_t last;
	char msg[128];
} __attribute__((packed)) packet_multi_t;

typedef struct {
	uint16_t type;
	time_t time_rem;
	union {
		task_travel_t travel;
	};
} __attribute__((packed)) packet_task_t;

typedef struct {
	uint16_t type;
	union {
		packet_version_t version;
		packet_auth_t auth;
		packet_error_t error;
		packet_player_t player;
		packet_cmd_t cmd;
		packet_multi_t multi;
		packet_task_t task;
	};
} __attribute__((packed)) packet_t;


// Common functions
int toint(char *);
int tosint(char *, int *);
int tohex(char *);

char *display_time(char *, int);
char *current_time();
time_t get_time();

int random_range(int, int);

int str_explode(char *, char **, char);
int str_explode_max(char *, char **, char, int);
char *str_explode_next(char *, char);

int char_eq(char *, char *);

#ifdef CONSOLE_H

// Not the best solution...

void print_player_profile(packet_player_t *p) {
	cprintf("!C--- %s's profile ---\n", p->name);
	cprintf("Level\t%d,\t%d XP\t %d HP, Movement speed:\t%d\n", p->level, p->xp, p->hp, p->movement_speed);
	cprintf("Position:\t%d;%d\n", p->x, p->y);
	if ( p->flags & AD_PLAYER_FLAG_RCON ) {
		cprintf("%s has RCON permission.\n", p->name);
	}
}

#endif

#endif
