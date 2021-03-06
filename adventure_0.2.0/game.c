#include "game.h"
#include "database.h"
#include "socket.h"
#include "console.h"
#include <string.h>
#include <math.h>

void error_packet(packet_t *packet, uint16_t code, uint8_t flags, char *msg) {
	memset(packet, 0, sizeof(packet_t));
	packet->type = AD_PACKET_ERROR;
	packet->error.code = code;
	packet->error.flags = flags;
	if ( msg != NULL ) {
		sprintf(packet->error.msg, "%s", msg);
	}
}

void packet_ok(packet_t *packet) {
	error_packet(packet, AD_ERROR_OK, 0, NULL);
}

player_info_t *players;

int player_create(char *name, char *password) {
	if ( char_eq(name, "admin") || char_eq(name, "root") || strlen(name) >= 16 ) {
		return 1;
	}
	for ( int i = 0; name[i]; i++ ) {
		char c = name[i];
		if ( !RANGE(c, 'a', 'z') && !RANGE(c, 'A', 'Z') && !RANGE(c, '0', '9') && c != '_' && c != '-' && c != '.' ) {
			return 1;
		}
	}
	player_database_t *new_player = &player_database[player_database_n];
	strcpy(new_player->name, name);
	memcpy(new_player->password_hash, password, sizeof(char) * 16);
	// fill up properties
	double r = 100 + 400 * (double)rand() / RAND_MAX;
	double fi = 2 * 3.14159265359 * (double) rand() / RAND_MAX;
	new_player->x = (int)( r * cos(fi) );
	new_player->y = (int)( r * sin(fi) );
	new_player->hp = 100;
	new_player->movement_speed = 1;
	player_database_n++;
	return database_backup();
}

int get_player(char *name) {
	for ( int i = 0; i < player_database_n; i++ ) {
		if ( char_eq(player_database[i].name, name) ) {
			return i;
		}
	}
	return -1;
}

int player_move(int db_idx, short x, short y) {
	if ( !RANGE(db_idx, 0, player_database_n) || !RANGE(x, -500, 500) || !RANGE(y, -500, 500) ) {
		return 1;
	}
	player_database[db_idx].x = x;
	player_database[db_idx].y = y;
	player_database[db_idx].task.type = AD_TASK_IDLE;
	char msg[128];
	sprintf(msg, "%sYou arrived to area %d;%d.", CL_FRONT_GREEN, x, y);
	packet_t p;
	memset(&p, 0, sizeof(packet_t));
	error_packet(&p, AD_ERROR_OK, AD_ERROR_FLAG_MSG, msg);
	player_send(db_idx, &p);
	return 0;
}


void player_close(player_info_t *player) {
	socket_close(player->socket);
	if ( player == players ) {
		players = player->next;
	} else {
		player->prev->next = player->next;
	}
	free(player);
}

int player_socket_send(player_info_t *player, packet_t *packet) {
	if ( socket_send(player->socket, (char *)packet, sizeof(packet_t)) < 0 ) {
		player_close(player);
		return 1;
	}
	return 0;
}

int player_send_im(int idx, packet_t *packet) {
	int c = 0;
	player_info_t *p = players;
	while ( p ) {
		if ( p->player_db == idx ) {
			if ( player_socket_send(p, packet) == 0 ) {
				c++;
			}
		}
		p = p->next;
	}
	return c;
}

int player_send(int id, packet_t *packet) {
	int r = player_send_im(id, packet);
	if ( r == 0 ) {
		inbox_t *msg = (inbox_t *)malloc(sizeof(inbox_t));
		memcpy(&msg->packet, packet, sizeof(packet_t));
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
	return r;
}

int player_check_inbox(int idx) {
	inbox_t *p = player_inbox[idx];
	while ( p ) {
		if ( player_send_im(idx, &p->packet) <= 0 ) {
			return 0;
		}
		player_inbox[idx] = p->next;
		free(p);
		p = player_inbox[idx];
	}
	return 0;
}

int player_send_msg(int target, char *sender, char *msg) {
	packet_t packet;
	error_packet(&packet, AD_ERROR_OK, AD_ERROR_FLAG_MSG, NULL);
	snprintf(packet.error.msg, 128, "[%s%s%s] %s", CL_FRONT_CYAN, sender, CL_FRONT_DEFAULT, msg);
	return player_send(target, &packet);
}

void player_send_fragments(player_info_t *player, char *str, packet_t *packet) {
	while ( 1 ) {
		memset(packet, 0, sizeof(packet_t));
		packet->type = AD_PACKET_MULTI;
		strncpy(packet->multi.msg, str, 127);
		packet->multi.msg[127] = '\0';
		if ( strlen(str) > 127 ) {
			player_socket_send(player, packet);
			str += 127;
		} else {
			packet->multi.last = 1;
			break;
		}
	}
}

void player_broadcast(packet_t *packet) {
	player_info_t *p = players;
	while ( p ) {
		player_socket_send(p, packet);
		p = p->next;
	}
}

int player_can_meet(int a, int b) {
	return player_database[a].x == player_database[b].x && player_database[a].y == player_database[b].y
		&& player_database[a].task.type == AD_TASK_IDLE && player_database[b].task.type == AD_TASK_IDLE;
}

double get_player_travel_distance(int idx) {
	int x = player_database[idx].x - player_database[idx].task.travel.x;
	int y = player_database[idx].y - player_database[idx].task.travel.y;
	return sqrt(x * x + y * y);
}

void player_list_invertory(player_database_t *player, char *str) {
	int i = 0;
	for ( int f = 0; f < 64; f++ ) {
		if ( player->invertory[f].stack > 0 ) {
			i += sprintf(str + i, "%5dx %s\n", player->invertory[f].stack, item_database[player->invertory[f].item_id].name);
		}
	}
	if ( i == 0 ) {
		sprintf(str, "No items found.");
	} else {
		str[i - 1] = '\0';
	}
}


int player_give_item(int player, int item, int q, char *name) {
	int s = 0;
	for ( int i = 0; i < 64; i++ ) {
		if ( player_database[player].invertory[i].stack > 0 && player_database[player].invertory[i].item_id == item ) {
			if ( q > 0 || player_database[player].invertory[i].stack > -q ) {
				player_database[player].invertory[i].stack += q;
			} else {
				player_database[player].invertory[i].stack = 0;
			}
			s = 1;
			break;
		}
	}
	if ( !s ) {
		for ( int i = 0; i < 64; i++ ) {
			if ( player_database[player].invertory[i].stack == 0 ) {
				player_database[player].invertory[i].item_id = item;
				player_database[player].invertory[i].stack  = q;
				s = 1;
				break;
			}
		}
	}
	if ( s ) {
		if ( name != NULL ) {
			packet_t packet;
			error_packet(&packet, AD_ERROR_OK, AD_ERROR_FLAG_MSG, NULL);
			sprintf(packet.error.msg, "%sYou were given %dx %s by %s.", CL_FRONT_BLUE, q, item_database[item].name, name);
			player_send(player, &packet);
		}
		return 1;
	}
	return 0;
}

int player_count_item(int player, int item_id) {
	for ( int i = 0; i < 64; i++ ) {
		if ( player_database[player].invertory[i].stack > 0 && player_database[player].invertory[i].item_id == item_id ) {
			return player_database[player].invertory[i].stack;
		}
	}
	return 0;
}

int get_item(char *name) {
	for ( int i = 0; i < item_database_n; i++ ) {
		if ( char_eq(item_database[i].name, name) ) {
			return i;
		}
	}
	return -1;
}
