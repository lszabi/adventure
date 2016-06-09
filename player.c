#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "database.h"

static player_t *players = NULL;

// Utility functions for players

int name_ok(char *name) {
	if ( !strcmp_i(name, "admin") || !strcmp_i(name, "root") || strlen(name) > 16 ) {
		return 0;
	}
	for ( int i = 0; name[i]; i++ ) {
		if ( !is_char(name[i]) ) {
			return 0;
		}
	}
	return 1;
}

// Functions to handle players

player_t *new_player() {
	return (player_t *)calloc(sizeof(player_t), 1);
}

void build_player(object_prop_t *p) {
	player_t *player = new_player();
	while ( p ) {
		if ( !strcmp(p->name, "name") && p->type == TYPE_STR ) {
			if ( name_ok(p->svalue) && !player->name && !search_player(p->svalue) ) {
				player->name = p->svalue;
			}
		} else if ( !strcmp(p->name, "password") && p->type == TYPE_STR ) {
			player->password = p->svalue;
		} else if ( !strcmp(p->name, "x") && p->type == TYPE_INT ) {
			if ( p->ivalue >= -500 && p->ivalue <= 500 ) {
				player->x = (short)p->ivalue;
			}
		} else if ( !strcmp(p->name, "y") && p->type == TYPE_INT ) {
			if ( p->ivalue >= -500 && p->ivalue <= 500 ) {
				player->y = (short)p->ivalue;
			}
		} else if ( !strcmp(p->name, "xp") && p->type == TYPE_INT ) {
			if ( p->ivalue >= 0 ) {
				player->xp = p->ivalue;
			}
		} else if ( !strcmp(p->name, "level") && p->type == TYPE_INT ) {
			if ( p->ivalue >= 0 ) {
				player->level = p->ivalue;
			}
		} else if ( !strcmp(p->name, "hp") && p->type == TYPE_INT ) {
			if ( p->ivalue >= 0 ) {
				player->hp = p->ivalue;
			}
		} else if ( !strcmp(p->name, "movement_speed") && p->type == TYPE_INT ) {
			if ( p->ivalue >= 0 ) {
				player->movement_speed = p->ivalue;
			}
		} else if ( !strcmp(p->name, "flags") && p->type == TYPE_HEX ) {
			player->flags = p->hvalue;
		} else if ( p->type == TYPE_STR ) {
			free(p->svalue);
		}
		free(p->name);
		object_prop_t *f = p;
		p = p->next;
		free(f);
	}
	player_t *f = players;
	if ( !f ) {
		players = player;
	} else {
		while ( f->next ) {
			f = f->next;
		}
		f->next = player;
	}
}

player_t *search_player(char *name) {
	player_t *p = players;
	while ( p ) {
		if ( !strcmp_i(p->name, name) ) {
			return p;
		}
		p = p->next;
	}
	return NULL;
}
