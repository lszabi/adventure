#include "common.h"
#include "database.h"

/*

	Adventure Admin Tools
	by L Szabi 2013

*/

const int ver_maj = 0, ver_min = 1, ver_rev = 6;

char buf[AD_BUF];
extern player_db_t *players_db;

int main() {
	printf("Adventure Admin Tools %d.%d.%d\n", ver_maj, ver_min, ver_rev);
	printf("by L Szabi - 2013\n\n");
	int done = 0;
	while ( !done ) {
		printf("Starting database...\n");
		int ret = start_db(1);
		switch ( ret ) {
			case 0:
				done = 1;
			break;
			case 1:
				printf("Failed to open player database!\n");
			break;
			case 2:
				printf("Failed to open item database!\n");
			break;
		}
		if ( !done ) {
			get_input("Do you want me to fix this error? ( y/n ) ( This can cause data loss! )\n", buf);
			if ( buf[0] == 'y' ) {
				init_db(ret);
			} else {
				printf("Ok, I'll just quit then...\n");
				return 0;
			}
		}
	}
	printf("%i players\n", players_n);
	done = 0;
	while ( !done ) {
		printf("Select database:\n");
		printf(" p - Players\n");
		printf(" i - Items\n");
		printf(" q - Exit\n");
		get_input("", buf);
		switch ( buf[0] ) {
			case 'p':
				printf("Players database\n");
				printf(" l - List players\n");
				printf(" v - View player attributes\n");
				printf(" m - Modify player\n");
				printf(" r - Remove player\n");
				get_input("", buf);
				char c = buf[0];
				switch ( c ) {
					case 'l':
						{
							int i, f = 0;
							for ( i = 0; i < players_n; i++ ) {
								printf("%s", players_db[i].name);
								int j = strlen(players_db[i].name);
								while ( j < 16 ) {
									printf(" ");
									j++;
								}
								f++;
								if ( f == 3 ) {
									f = 0;
									printf("\n");
								} else {
									printf("\t");
								}
							}
							if ( f != 0 ) {
								printf("\n");
							}
							printf("%i players.\n", players_n);
						}
					break;
					case 'v':
					case 'r':
					case 'm':
						{
							get_input("Name: ", buf);
							int i, f = -1;
							for ( i = 0; i < players_n; i++ ) {
								if ( stricmp(buf, players_db[i].name) == 0 ) {
									f = i;
									break;
								}
							}
							if ( f == -1 ) {
								printf("Unknown player.\n");
							} else {
								if ( c == 'r' ) {
									players_db[f].name[0] = '\0';
									printf("Done.\n");
								} else if ( c == 'm' ) {
									printf("What do you want to modify?\n");
									printf(" m - Max HP\n");
									printf(" a - ATK\n");
									printf(" d - DEF\n");
									printf(" s - SPD\n");
									printf(" c - DMG\n");
									printf(" h - HP\n");
									printf(" t - Movement speed\n");
									printf(" e - XP\n");
									printf(" l - Level\n");
									get_input("", buf);
									char a = buf[0];
									get_input("Value: ", buf);
									i = toint(buf);
									if ( i == -1 ) {
										printf("Invalid number!\n");
									} else {
										switch ( a ) {
											case 'm':
												players_db[f].max_hp = i;
											break;
											case 'a':
												players_db[f].atk = i;
											break;
											case 'd':
												players_db[f].def = i;
											break;
											case 's':
												players_db[f].spd = i;
											break;
											case 'c':
												players_db[f].dmg = i;
											break;
											case 'h':
												players_db[f].hp = i;
											break;
											case 't':
												players_db[f].mv_spd = i;
											break;
											case 'e':
												players_db[f].xp = i;
											break;
											case 'l':
												players_db[f].lvl = i;
											break;
										}
										printf("Done!\n");
									}
								} else if ( c == 'v' ) {
									printf("%s's character at %i;%i:\n", players_db[f].name, players_db[f].x, players_db[f].y);
									printf("Level %i, %i xp\n", players_db[f].lvl, players_db[f].xp);
									printf("ATK\t%i\tDEF\t%i\n", players_db[f].atk, players_db[f].def);
									printf("SPD\t%i\tDMG\t%i\n", players_db[f].spd, players_db[f].dmg);
									printf("HP\t%i\tMAX HP\t%i\n", players_db[f].hp, players_db[f].max_hp);
									printf("Movement speed: %i\n", players_db[f].mv_spd);
									if ( players_db[f].flags & AD_FLAG_RCON ) {
										printf("Player is an RCON admin.\n");
									}
									if ( players_db[f].flags & AD_FLAG_BANNED ) {
										printf("Player is BANNED.\n");
									}
								}
							}
						}
					break;
				}
			break;
			case 'q':
				done = 1;
			break;
		}
	}
	close_db(1);
	printf("Bye!\n");
	return 0;
}
