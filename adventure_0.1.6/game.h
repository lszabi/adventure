#ifndef GAME_H
#define GAME_H

// Adventure game by L Szabi 2013

#include "common.h"
#include "database.h"

double get_dist(short, short, short, short);
void get_div_point(short, short, short, short, double, double, short *, short *);
void init_player(player_db_t *);

#endif // GAME_H
