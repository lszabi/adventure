#ifndef GAME_H
#define GAME_H

#include "common.h"
#include "database.h"
#include <time.h>

typedef struct player_info {
	int socket;
	int stage;
	int player_db;
	time_t last_process;
	struct player_info *next;
	struct player_info *prev;
} player_info_t;

extern player_info_t *players;

void error_packet(packet_t *, uint16_t, uint8_t, char *);
void packet_ok(packet_t *);

int player_create(char *, char *);
int get_player(char *);

void player_close(player_info_t *);
int player_socket_send(player_info_t *, packet_t *);
int player_send_im(int, packet_t *);
int player_send(int, packet_t *);
int player_check_inbox(int);
int player_send_msg(int, char *, char *);
void player_send_fragments(player_info_t *, char *, packet_t *);
void player_broadcast(packet_t *);

int player_move(int, short, short);
int player_can_meet(int, int);

double get_player_travel_distance();

void player_list_invertory(player_database_t *, char *);
int player_give_item(int, int, int, char *);
int player_count_item(int, int);

int get_item(char *);

#endif