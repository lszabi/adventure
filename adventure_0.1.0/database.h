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
	uint8_t task_id;
	time_t task_started;
	uint32_t task_arg;
} __attribute__((packed)) task_t;

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
} __attribute__((packed)) player_database_t;

typedef struct {
	uint32_t id;
	packet_t packet;
} __attribute__((packed)) inbox_database_t;

typedef struct inbox {
	packet_t packet;
	struct inbox *next;
} inbox_t;

extern player_database_t *player_database;
extern int player_database_n;

extern inbox_t **player_inbox;

int database_open();
int database_close();
int database_backup();

#endif