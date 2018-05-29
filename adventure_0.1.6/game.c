#include "game.h"
#include <math.h>

// Adventure game by L Szabi 2013

// Coordinate-geometry distance formula
double get_dist(short x1, short y1, short x2, short y2) {
	return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

// Dividing point in p:q ratio for (x1; y1)(x2; y2) section
// r = ( ( q * x1 + p * x2) / ( p + q ); ( q * y1 + p * y2 ) / ( p + q ) ) where |(x1; y1)(r)| : |(r)(x2; y2)| = p : q
void get_div_point(short x1, short y1, short x2, short y2, double p, double q, short *x3, short *y3) {
	*x3 = ( q * x1 + p * x2 ) / ( p + q );
	*y3 = ( q * y1 + p * y2 ) / ( p + q );
}

// Initialize player skillz
void init_player(player_db_t *player) {
	player->max_hp = 100;
	player->atk = 10;
	player->def = 10;
	player->dmg = 10;
	player->spd = 10;
	player->hp = 100;
	player->mv_spd = 100;
}
