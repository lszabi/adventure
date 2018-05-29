#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/*
 * Adventure Database handler
 */

/*
each file:
uint32_t block_size;
uint32_t block_count;
block_size blocks * block_count;
*/

player_database_t *player_database;
int player_database_n = 0;

inbox_t **player_inbox;

item_database_t *item_database;
int item_database_n = 0;

static void make_dir(char *name) {
#ifdef WIN32
	mkdir(name);
#else
	mkdir(name, 0777);
#endif
}

static FILE *open_file(char *filename) {
	FILE *f;
	if ( access(filename, F_OK) != 0 ) {
		f = fopen(filename, "wb+");
		if ( f != NULL ) {
			file_header_t head;
			head.block_count = 0;
			head.block_size = 0;
			fwrite(&head, sizeof(file_header_t), 1, f);
			rewind(f);
			return f;
		}
		return NULL;
	}
	if ( access(filename, F_OK | R_OK | W_OK) != 0 ) {
		return NULL;
	}
	f = fopen(filename, "rb");
	return f;
}

static int read_header(FILE *file, file_header_t *file_header) {
	if ( fread(file_header, sizeof(file_header_t), 1, file) != 1 ) {
		return -1;
	}
	return 0;
}

static int read_file(FILE *file, void *destination, size_t size, file_header_t *file_header) {
	if ( file_header->block_count > 0 ) {
		if ( file_header->block_size > size ) {
			return -1;
		}
		void *blocks = malloc(file_header->block_count * file_header->block_size);
		if ( fread(blocks, file_header->block_size, file_header->block_count, file) != file_header->block_count ) {
			free(blocks);
			return -1;
		}
		for ( int i = 0; i < file_header->block_count; i++ ) {
			memcpy(destination + i * size, blocks + i * file_header->block_size, file_header->block_size);
		}
		free(blocks);
	}
	return 0;
}

static int read_players() {
	file_header_t file_header;
	FILE *file_players = open_file("db/players.bin");
	if ( file_players == NULL ) {
		return -1;
	}
	if ( read_header(file_players, &file_header) < 0 ) {
		fclose(file_players);
		return -1;
	}
	player_database_n = file_header.block_count;
	player_database = (player_database_t *)malloc(sizeof(player_database_t) * ( player_database_n + 1 ));
	memset(player_database, 0, sizeof(player_database_t) * ( player_database_n + 1 ));
	if ( read_file(file_players, player_database, sizeof(player_database_t), &file_header) < 0 ) {
		free(player_database);
		fclose(file_players);
		return -1;
	}
	fclose(file_players);
	return 0;
}

static int read_inbox() {
	FILE *f = open_file("db/inbox.bin");
	if ( f == NULL ) {
		return -1;
	}
	file_header_t file_header;
	if ( read_header(f, &file_header) < 0 ) {
		fclose(f);
		return -1;
	}
	player_inbox = (inbox_t **)malloc(sizeof(inbox_t *) * ( player_database_n + 1 ));
	memset(player_inbox, 0, sizeof(inbox_t *) * ( player_database_n + 1 ));
	inbox_database_t *f_inbox = (inbox_database_t *)malloc(sizeof(inbox_database_t) * file_header.block_count);
	if ( read_file(f, f_inbox, sizeof(inbox_database_t), &file_header) < 0 ) {
		free(player_inbox);
		free(f_inbox);
		fclose(f);
		return -1;
	}
	for ( int i = 0; i < file_header.block_count; i++ ) {
		int id = f_inbox[i].id;
		inbox_t *msg = (inbox_t *)malloc(sizeof(inbox_t));
		memcpy(&msg->packet, &f_inbox[i].packet, sizeof(packet_t));
		msg->next = 0;
		if ( player_inbox[id] == 0 ) {
			player_inbox[id] = msg;
		} else {
			inbox_t *p = player_inbox[id];
			while ( p->next ) {
				p = p->next;
			}
			p->next = msg;
		}
	}
	free(f_inbox);
	fclose(f);
	return 0;
}

static int read_items() {
	FILE *f = open_file("db/items.bin");
	if ( f == NULL ) {
		return -1;
	}
	file_header_t file_header;
	if ( read_header(f, &file_header) < 0 ) {
		fclose(f);
		return -1;
	}
	item_database = (item_database_t *)malloc(sizeof(item_database_t) * file_header.block_count);
	if ( read_file(f, item_database, sizeof(item_database_t), &file_header) < 0 ) {
		free(item_database);
		fclose(f);
		return -1;
	}
	item_database_n = file_header.block_count;
	fclose(f);
	return 0;
}

static int read_world() {
	if ( read_players() < 0 ) {
		return -1;
	}
	if ( read_inbox() < 0 ) {
		return -1;
	}
	return 0;
}

static int read_static() {
	if ( read_items() < 0 ) {
		return -1;
	}
	return 0;
}

static int close_players() {
	FILE *file_players = fopen("db/players.bin", "wb");
	if ( file_players == NULL ) {
		return -1;
	}
	file_header_t file_header;
	file_header.block_count = player_database_n;
	file_header.block_size = sizeof(player_database_t);
	if ( fwrite(&file_header, sizeof(file_header_t), 1, file_players) != 1 ) {
		fclose(file_players);
		return -1;
	}
	if ( fwrite(player_database, file_header.block_size, file_header.block_count, file_players) != file_header.block_count ) {
		fclose(file_players);
		return -1;
	}
	free(player_database);
	fclose(file_players);
	player_database_n = 0;
	return 0;
}

static int close_inbox() {
	FILE *f = fopen("db/inbox.bin", "wb");
	if ( f == NULL ) {
		return -1;
	}
	file_header_t file_header;
	file_header.block_count = 0;
	file_header.block_size = sizeof(inbox_database_t);
	for ( int i = 0; i < player_database_n; i++ ) {
		inbox_t *p = player_inbox[i];
		while ( p ) {
			file_header.block_count++;
			p = p->next;
		}
	}
	if ( fwrite(&file_header, sizeof(file_header_t), 1, f) != 1 ) {
		fclose(f);
		return -1;
	}
	inbox_database_t *f_inbox = (inbox_database_t *)malloc(sizeof(inbox_database_t) * file_header.block_count);
	memset(f_inbox, 0, sizeof(inbox_database_t) * file_header.block_count);
	int k = 0;
	for ( int i = 0; i < player_database_n; i++ ) {
		inbox_t *p = player_inbox[i];
		while ( p ) {
			f_inbox[k].id = i;
			memcpy(&f_inbox[k].packet, &p->packet, sizeof(packet_t));
			k++;
			inbox_t *t = p;
			p = p->next;
			free(t);
		}
	}
	if ( fwrite(f_inbox, file_header.block_size, file_header.block_count, f) != file_header.block_count ) {
		free(f_inbox);
		fclose(f);
		return -1;
	}
	free(f_inbox);
	free(player_inbox);
	fclose(f);
	return 0;
}

static int close_items() {
	free(item_database);
	item_database_n = 0;
	return 0;
}

static int close_world() {
	if ( close_inbox() < 0 ) {
		return -1;
	}
	if ( close_players() < 0 ) {
		return -1;
	}
	return 0;
}

static int close_static() {
	if ( close_items() < 0 ) {
		return -1;
	}
	return 0;
}

int database_open() {
	make_dir("db");
	if ( read_world() < 0 ) {
		return -1;
	}
	if ( read_static() < 0 ) {
		return -1;
	}
	return 0;
}

int database_close() {
	if ( close_world() < 0 ) {
		return -1;
	}
	if ( close_static() < 0 ) {
		return -1;
	}
	return 0;
}

int database_backup() {
	remove("db/players.bak");
	remove("db/inbox.bak");
	if ( rename("db/players.bin", "db/players.bak") != 0 ) {
		return -1;
	}
	if ( rename("db/inbox.bin", "db/inbox.bak") != 0 ) {
		return -1;
	}
	if ( close_world() < 0 ) {
		return -1;
	}
	if ( read_world() < 0 ) {
		return -1;
	}
	return 0;
}
