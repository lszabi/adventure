#ifndef DATABASE_H
#define DATABASE_H

#include "common.h"
#include <time.h>
#include <stdint.h>

typedef struct {
	uint32_t block_size;
	uint32_t block_count;
} __attribute__((packed)) file_header_t;

typedef struct {
	uint16_t type;
	time_t started;
	union {
		task_travel_t travel;
	};
} __attribute__((packed)) task_t;

typedef struct {
	uint32_t item_id;
	uint32_t stack;
} __attribute__((packed)) invertory_t;

typedef struct {
	char password_hash[16];
	char name[16];
	short x;
	short y;
	int xp;
	int level;
	int hp;
	int movement_speed;
	uint32_t flags;
	uint32_t reserved[16];
	task_t task;
	invertory_t invertory[64];
} __attribute__((packed)) player_database_t;

typedef struct {
	uint32_t id;
	packet_t packet;
} __attribute__((packed)) inbox_database_t;

typedef struct inbox {
	packet_t packet;
	struct inbox *next;
} inbox_t;

typedef struct {
	char name[16];	
} __attribute__((packed)) item_database_t;

extern player_database_t *player_database;
extern int player_database_n;

extern inbox_t **player_inbox;

extern item_database_t *item_database;
extern int item_database_n;

int database_open();
int database_close();
int database_backup();

#endif