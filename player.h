#ifndef AD_PLAYER_H
#define AD_PALYER_H

#include <stdint.h>
#include "database.h"

typedef struct player {
	char *name;
	char *password;
	short x, y;
	uint32_t xp, level, hp, movement_speed, flags;
	struct player *next;
} player_t;

player_t *new_player();
void build_player(object_prop_t *);

player_t *search_player(char *);

#endif