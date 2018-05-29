#include "common.h"
#include "database.h"
#include "game.h"
#include <pthread.h> // -lpthread

/*

	Adventure Server
	by L Szabi 2013
	
*/

//#define NOADMIN

const int ver_maj = 0, ver_min = 1, ver_rev = 6;

#define AD_MSG_LIMIT 120
#define AD_RETRY_LIMIT 3
#define AD_PLAYER_DELAY 0.1
#define AD_MAIN_DELAY 0.5

typedef struct {
	int id;
	int state;
	int retry;
	int socket;
	user_t *player_db;
	pthread_t thr;
} player_t;

player_t *players;

static int term = 0, sck, max_players, paused = 0;
static FILE *log;

// Get player
static int get_player(char *name) {
	int i;
	for ( i = 0; i < max_players; i++ ) {
		if ( players[i].player_db != NULL && !strcmp(players[i].player_db->db->name, name) ) {
			return i;
		}
	}
	return -1;
}

// Broadcast message
static int bcast_msg(unsigned char *msg, int len) {
	int i, f = 0;
	for ( i = 0; i < max_players; i++ ) {
		if ( players[i].player_db != NULL ) {
			push_list(msg, len, players[i].player_db->todo);
			f++;
		}
	}
	return f;
}

// Handle close signal sent by os
void signal_close(int n) {
	signal(SIGINT, signal_close);
	signal(SIGTERM, signal_close);
	if ( !term ) {
		printf("\nClose signal detected.\n");
		fprintf(log, "Close signal detected.\n");
		term = 1;
	} else {
		printf("Patience, please...\n");
	}
}

// Process admin commands
static void process_admin_command(char *buf, char *buf_out, char *admin_name) {
	int tmp = 0;
	user_t *p = NULL;
	switch ( buf[0] ) {
		case 'q':
			sprintf(buf_out, "Server going down.");
			fprintf(log, "Shutdown started by %s.\n", admin_name);
			term = 1;
		break;
		case 's':
			convert_sep(buf + 2);
			tmp = get_player(buf + 2);
			if ( tmp == -1 ) {
				sprintf(buf_out, "Unknown player.");
			} else {
				int l = strlen(buf + 2);
				buf += 3 + l;
				l = strlen(buf);
				if ( l > AD_MSG_LIMIT ) {
					sprintf(buf_out, "Message too long.");
				} else {
					sprintf(buf_out, "%c[%s] %s", AD_MSG, admin_name, buf);
					push_list((unsigned char *)buf_out, strlen(buf_out) + 1, players[tmp].player_db->todo);
					sprintf(buf_out, "Message send scheduled.");
				}
			}
		break;
		case 'b':
			tmp = convert_sep(buf + 2);
			if ( tmp > AD_MSG_LIMIT ) {
				sprintf(buf_out, "Message too long.");
			} else {
				sprintf(buf_out, "%c[%s] %s", AD_MSG, admin_name, buf + 2);
				int f = bcast_msg((unsigned char *)buf_out, strlen(buf_out) + 1);
				sprintf(buf_out, "Message broadcast to %i players scheduled.", f);
			}
		break;
		case 'p':
			if ( paused ) {
				paused = 0;
				sprintf(buf_out, "Server unpaused.");
				printf("Server unpaused.\n");
			} else {
				paused = 1;
				sprintf(buf_out, "Server paused.");
				printf("Server paused.\n");
			}
		break;
		case 't':
			convert_sep(buf + 2);
			p = get_player_db(buf + 2);
			if ( p == NULL ) {
				sprintf(buf_out, "Unknown player.");
			} else {
				short x = 0, y = 0;
				int err = 0;
				int i = strlen(buf + 2) + 3;
				int l = convert_sep(buf + i);
				x = tosint(buf + i, &err);
				if ( err ) {
					sprintf(buf_out, "Invalid number x.");
					return;
				}
				if ( x > 500 || x < -500 ) {
					sprintf(buf_out, "Invalid coordinate x.");
					return;
				}
				i += l + 1;
				y = tosint(buf + i, &err);
				if ( err ) {
					sprintf(buf_out, "Invalid number y.");
					return;
				}
				if ( y > 500 || y < -500 ) {
					sprintf(buf_out, "Invalid coordinate y.");
					return;
				}
				if ( p->db->x != x || p->db->y != y ) {
					p->db->task = AD_T_IDLE;
					p->db->x = x;
					p->db->y = y;
					sprintf(buf_out, "Done.");
					sprintf(buf, "%c%s teleported you to %i;%i.", AD_MSG, admin_name, x, y);
					fprintf(log, "%s teleported %s to %i:%i.\n", admin_name, p->db->name, x, y);
					push_list((unsigned char *)buf, strlen(buf) + 1, p->todo);
				} else {
					sprintf(buf_out, "The player is right there.");
				}
			}
		break;
		case 'u':
			p = get_player_db(buf + 2);
			if ( p == NULL ) {
				sprintf(buf_out, "Unknown player.");
			} else {
				sprintf(buf_out, "%s's character at %i;%i:\n", p->db->name, p->db->x, p->db->y);
				sprintf(buf_out + strlen(buf_out), "Level %i, %i xp\n", p->db->lvl, p->db->xp);
				sprintf(buf_out + strlen(buf_out), "ATK\t%i\tDEF\t%i\n", p->db->atk, p->db->def);
				sprintf(buf_out + strlen(buf_out), "SPD\t%i\tDMG\t%i\n", p->db->spd, p->db->dmg);
				sprintf(buf_out + strlen(buf_out), "HP\t%i\tMAX HP\t%i\n", p->db->hp, p->db->max_hp);
				sprintf(buf_out + strlen(buf_out), "Movement speed: %i", p->db->mv_spd);
				if ( p->db->flags & AD_FLAG_RCON ) {
					sprintf(buf_out + strlen(buf_out), "\nPlayer is an RCON admin.");
				}
				if ( p->db->flags & AD_FLAG_BANNED ) {
					sprintf(buf_out + strlen(buf_out), "\nPlayer is BANNED.");
				}
			}
		break;
		case 'f':
			convert_sep(buf + 2);
			p = get_player_db(buf + 2);
			if ( p == NULL ) {
				sprintf(buf_out, "Unknown player.");
			} else {
				int l = strlen(buf + 2) + 3;
				if ( strcmp(buf + l + 2, "yes") == 0 ) {
					switch ( buf[l] ) {
						case 'r':
							p->db->flags |= AD_FLAG_RCON;
							sprintf(buf_out, "%s is RCON now.", p->db->name);
							fprintf(log, "%s made %s an RCON.\n", admin_name, p->db->name);
						break;
						case 'b':
							p->db->flags |= AD_FLAG_BANNED;
							sprintf(buf_out, "%s is BANNED now.", p->db->name);
							fprintf(log, "%s banned %s.\n", admin_name, p->db->name);
						break;
						default:
							sprintf(buf_out, "No operation executed.");
						break;
					}
				} else {
					switch ( buf[l] ) {
						case 'r':
							p->db->flags &= ~( AD_FLAG_RCON );
							sprintf(buf_out, "%s is no longer RCON.", p->db->name);
							fprintf(log, "%s made %s an non-RCON.\n", admin_name, p->db->name);
						break;
						case 'b':
							p->db->flags &= ~( AD_FLAG_BANNED );
							sprintf(buf_out, "%s is unbanned.", p->db->name);
							fprintf(log, "%s unbanned %s.\n", admin_name, p->db->name);
						break;
						default:
							sprintf(buf_out, "No operation executed.");
						break;
					}
				}
			}
		break;
		default:
			sprintf(buf_out, "No operation executed.");
		break;
	}
}

// Admin thread to handle stdin
void *thread_admin() {
	char buf[AD_BUF];
	char obuf[AD_BUF];
	while ( !term ) {
		get_input("", buf);
		process_admin_command(buf, obuf, "admin");
		printf("%s\n", obuf);
		obuf[0] = '\0';
	}
	return NULL;
}

// Handle packets received from clients
int player_handler(unsigned char *msg, player_t *player, int *len) {
	int tmp = 0;
	switch ( player->state ) {
		case AD_STATE_INIT:
			switch ( msg[0] ) {
				case AD_MAGIC_CLIENT:
					msg[0] = AD_VER;
					*len = 1;
					player->state = AD_STATE_VER;
				break;
				case AD_LOGOUT:
					msg[0] = AD_CLOSE;
					*len = 1;
					return 1;
				break;
				default:
					msg[1] = msg[0];
					msg[0] = AD_ERR_MAGIC;
					*len = 2;
					return 1;
				break;
			}
		break;
		case AD_STATE_VER:
			if ( get_int(msg) == ver_maj && get_int(msg + 4) == ver_min && get_int(msg + 8) == ver_rev ) {
				msg[0] = AD_ACK;
				*len = 1;
				player->state = AD_STATE_AUTH;
			} else {
				msg[0] = AD_ERR_VER;
				print_int(msg + 1, ver_maj);
				print_int(msg + 5, ver_min);
				print_int(msg + 9, ver_rev);
				*len = 13;
				return 1;
			}
		break;
		case AD_STATE_AUTH:
			switch ( msg[0] ) {
				case AD_OK:
					msg[0] = AD_AUTH;
					*len = 1;
				break;
				case AD_ERR_COMM:
					fprintf(log, "Did we send an invalid token? 0x%x in AD_STATE_AUTH.\n", (int)( msg[1] ));
				case AD_LOGOUT:
					msg[0] = AD_CLOSE;
					*len = 1;
					return 1;
				break;
				case AD_LOGIN:
					player->player_db = get_player_db((char *)(msg + 1));
					if ( player->player_db == NULL ) {
						if ( player->retry == AD_RETRY_LIMIT ) {
							msg[0] = AD_CLOSE;
							*len = 1;
							return 1;
						} else {
							msg[0] = AD_ERR_UNAME;
							*len = 1;
							player->retry++;
						}
					} else {
						msg[0] = AD_PASSW;
						*len = 1;
						player->state = AD_STATE_LOGIN;
					}
				break;
				case AD_REG:
					tmp = register_player((char *)(msg + 1));
					if ( tmp == 0 ) {
						player->player_db = get_player_db((char *)(msg + 1));
						init_player(player->player_db->db);
						msg[0] = AD_PASSW;
						*len = 1;
						player->state = AD_STATE_REG;
					} else if ( tmp == 1 ) {
						printf("Register limit reached, server needs to be restarted!\n");
						fprintf(log, "Register limit reached, server needs to be restarted!\n");
						msg[0] = AD_ERR;
						*len = 1;
					} else if ( tmp == 2 || tmp == 3 || tmp == 4 ) {
						if ( player->retry == AD_RETRY_LIMIT ) {
							msg[0] = AD_CLOSE;
							*len = 1;
							return 1;
						} else {
							msg[0] = AD_ERR_UNAME;
							*len = 1;
							player->retry++;
						}
					}
				break;
				default:
					msg[1] = msg[0];
					msg[0] = AD_ERR_COMM;
					*len = 2;
					return 1;
				break;
			}
		break;
		case AD_STATE_LOGIN:
			switch ( msg[0] ) {
				case AD_ENTPASSW:
					if ( !memcmp(player->player_db->db->passwd, msg + 1, sizeof(char) * 16) ) {
						fprintf(log, "Player %i is %s.\n", player->id, player->player_db->db->name);
						msg[0] = AD_ACK;
						*len = 1;
						player->state = AD_STATE_MAIN;
					} else {
						msg[0] = AD_ERR_PASSW;
						*len = 1;
						player->state = AD_STATE_AUTH;
						player->player_db = NULL;
					}
				break;
				default:
					msg[1] = msg[0];
					msg[0] = AD_ERR_COMM;
					*len = 2;
					return 1;
				break;
			}
		break;
		case AD_STATE_REG:
			switch ( msg[0] ) {
				case AD_CHPASSW:
					memcpy(player->player_db->db->passwd, msg + 1, sizeof(char) * 16);
					fprintf(log, "New player: %s, id %i.\n", player->player_db->db->name, player->id);
					printf("New player!\n");
					player->state = AD_STATE_MAIN;
					msg[0] = AD_ACK;
					*len = 1;
				break;
				default:
					msg[1] = msg[0];
					msg[0] = AD_ERR_COMM;
					*len = 2;
					return 1;
				break;
			}
		break;
		case AD_STATE_MAIN:
			switch ( msg[0] ) {
				case AD_ERR_COMM:
					fprintf(log, "Did we send an invalid token? 0x%x in AD_STATE_MAIN.\n", (int)( msg[1] ));
				case AD_LOGOUT:
					msg[0] = AD_CLOSE;
					*len = 1;
					return 1;
				break;
				case AD_CHPASSW:
					memcpy(player->player_db->db->passwd, msg + 1, sizeof(char) * 16);
					msg[0] = AD_ACK;
					*len = 1;
				break;
				case AD_UPD:
					if ( player->player_db != NULL && player->player_db->todo->len ) {
						if ( player->player_db->db->task != AD_T_IDLE ) {
							msg[0] = AD_MULTI;
							msg++;
							player->player_db->todo = pop_list(msg, len, player->player_db->todo);
							(*len)++;
						} else {
							player->player_db->todo = pop_list(msg, len, player->player_db->todo);
						}
						if ( msg[0] == AD_CLOSE ) {
							return 1;
						}
					} else {
						if ( player->player_db->db->task != AD_T_IDLE ) {
							msg[0] = AD_BUSY;
							msg[1] = player->player_db->db->task;
							print_int(msg + 2, player->player_db->time_rem);
							*len = 6;
						} else {
							msg[0] = AD_NOP;
							*len = 1;
						}
					}
				break;
				case AD_S_POS:
					msg[0] = AD_S_POS;
					print_short(msg + 1, player->player_db->db->x);
					print_short(msg + 3, player->player_db->db->y);
					*len = 5;
				break;
				case AD_SENDMSG:
					msg[AD_BUF - 1] = '\0';
					tmp = get_player((char *)(msg + 1));
					if ( tmp != -1 ) {
						if ( &(players[tmp]) != player ) {
							int start = strlen((char *)msg) + 1;
							if ( strlen((char *)(msg + start)) <= AD_MSG_LIMIT ) {
								if ( players[tmp].player_db->db->x == player->player_db->db->x 
									&& players[tmp].player_db->db->y == player->player_db->db->y 
									&& players[tmp].player_db->db->task != AD_T_TRAVEL
									&& player->player_db->db->task != AD_T_TRAVEL)
								{
									unsigned char buf[AD_BUF];
									sprintf((char *)buf, "%c[%s] %s", AD_MSG, player->player_db->db->name, (char *)(msg + start));
									push_list(buf, strlen((char *)buf), players[tmp].player_db->todo);
									sprintf((char *)msg, "%cMesssage sent.", AD_MSG);
								} else {
									sprintf((char *)msg, "%cThis player is not here.", AD_MSG);
								}
							} else {
								sprintf((char *)msg, "%cMessage too long", AD_MSG);
							}
						} else {
							sprintf((char *)msg, "%cForever alone...", AD_MSG);
						}
					} else {
						sprintf((char *)msg, "%cUnknown or offline player.", AD_MSG);
					}
					*len = strlen((char *)msg);
				break;
				case AD_TASK:
					switch ( msg[1] ) {
						case AD_T_TRAVEL:
							;
							int x = get_short(msg + 2);
							int y = get_short(msg + 4);
							if ( x <= 500 && x >= -500 && y <= 500 && y >= -500 ) {
								if ( player->player_db->db->x != x || player->player_db->db->y != y ) {
									if ( player->player_db->db->task == AD_T_IDLE ) {
										player->player_db->db->task = AD_T_TRAVEL;
										player->player_db->db->task_started = time(NULL);
										player->player_db->db->task_arg = ( x << 16 ) | y;
										sprintf((char *)msg, "%c%cYou begin your journey to the Unknown...", AD_MULTI, AD_MSG);
									} else {
										sprintf((char *)msg, "%cYou are a bit busy at the moment...", AD_MSG);
									}
								} else {
									sprintf((char *)msg, "%cNow you are here.", AD_MSG);
								}
							} else {
								sprintf((char *)msg, "%cInvalid coordinates.", AD_MSG);
							}
							*len = strlen((char *)msg);
						break;
						default:
							msg[0] = AD_ERR_COMM;
							*len= 1;
							return 1;
						break;
					}
				break;
				case AD_ABORT:
					if ( player->player_db->time_rem > 15 ) {
						switch ( player->player_db->db->task ) {
							case AD_T_IDLE:
								msg[0] = AD_NOP;
								*len = 1;
							break;
							case AD_T_TRAVEL:
								player->player_db->db->task = AD_T_IDLE;
								// s = v * t
								double dist_rem = player->player_db->db->mv_spd * ( (double)(player->player_db->time_rem) / 3600 );
								short x1 = player->player_db->db->x, y1 = player->player_db->db->y;
								short x2 = ( player->player_db->db->task_arg & 0xFFFF0000 ) >> 16, y2 = player->player_db->db->task_arg & 0xFFFF;
								double dist_done = get_dist(x1, y1, x2, y2) - dist_rem;
								short x3 = 0, y3 = 0;
								get_div_point(x1, y1, x2, y2, dist_done, dist_rem, &x3, &y3);
								player->player_db->db->x = x3;
								player->player_db->db->y = y3;
								msg[0] = AD_ACK;
								*len = 1;
							break;
							default:
							break;
						}
					} else { 
						msg[0] = AD_NOP;
						*len = 1;
					}
				break;
				case AD_S_PROF:
					msg[0] = AD_S_PROF;
					print_int(msg + 1, player->player_db->db->lvl);
					print_int(msg + 5, player->player_db->db->xp);
					print_int(msg + 9, player->player_db->db->atk);
					print_int(msg + 13, player->player_db->db->def);
					print_int(msg + 17, player->player_db->db->spd);
					print_int(msg + 21, player->player_db->db->dmg);
					print_int(msg + 25, player->player_db->db->hp);
					print_int(msg + 29, player->player_db->db->max_hp);
					print_int(msg + 33, player->player_db->db->mv_spd);
					print_int(msg + 37, player->player_db->db->flags);
					*len = 41;
				break;
				case AD_GETPLIST:
					{
						int i, f = 1;
						tmp = 1;
						*len = 1;
						if ( player->player_db->db->task == AD_T_TRAVEL ) {
							return 0;
						}
						unsigned char buf[AD_BUF];
						memset(buf, 0, sizeof(unsigned char) * AD_BUF);
						memset(msg, 0, sizeof(unsigned char) * AD_BUF);
						msg[0] = AD_PLIST;
						for ( i = 0; i < max_players; i++ ) {
							if ( players[i].player_db != NULL 
								&& players[i].player_db->db->x == player->player_db->db->x
								&& players[i].player_db->db->y == player->player_db->db->y
								&& players[i].player_db->db->task != AD_T_TRAVEL
								&& stricmp(players[i].player_db->db->name, player->player_db->db->name) != 0 )
							{
								int l = strlen(players[i].player_db->db->name) + 1;
								if ( l + tmp <= AD_BUF ) {
									sprintf((char *)( f ? msg : buf ) + tmp, "%s%c", players[i].player_db->db->name, '\0');
									tmp += l;
									if ( f ) {
										*len = tmp;
									}
								} else {
									if ( f ) {
										f = 0;
										buf[0] = AD_PLIST;
										*len = tmp;
										tmp = 1;
										i--;
									} else {
										push_list(buf, tmp, player->player_db->todo);
										memset(buf, 0, sizeof(unsigned char) * AD_BUF);
										buf[0] = AD_PLIST;
										*len = 1;
										i--;
									}
								}
							}
						}
						if ( !f ) {
							push_list(buf, tmp, player->player_db->todo);
						}
					}
				break;
				case AD_RCON_MSG:
					if ( player->player_db->db->flags & AD_FLAG_RCON ) {
						char buf[AD_BUF];
						process_admin_command((char *)(msg + 1), buf, player->player_db->db->name);
						sprintf((char *)msg, "%c%s", AD_MSG, buf);
						*len = strlen(buf) + 2;
					} else {
						sprintf((char *)msg, "%cNice try, but yo ain't no RCON.", AD_MSG);
						*len = strlen((char *)msg) + 1;
					}
				break;
				default:
					msg[0] = AD_ERR_COMM;
					*len= 1;
					return 1;
				break;
			}
		break;
	}
	return 0;
}

// Thread to handle connected players
void *thread_player(void *idx) {
	player_t *player = &(players[(int)idx]);
	int new_sck = player->socket;
	unsigned char msg[AD_BUF];
	fprintf(log, "Player %i connected.\n", player->id);
	int brk = 0;
	memset(msg, 0, sizeof(char) * AD_BUF);
	msg[0] = AD_MAGIC_SERVER;
	int ret = send(new_sck, (char *)msg, 1, 0);
	if ( ret <= 0 ) {
		brk = 1;
	}
	while ( !brk ) {
		// Receive response
		memset(msg, 0, sizeof(char) * AD_BUF);
		ret = recv(new_sck, (char *)msg, AD_BUF, 0);
		if ( ret <= 0 ) {
			fprintf(log, "Connection lost to player %i during recv.\n", player->id);
			break;
		}
		sleep(AD_PLAYER_DELAY);
		// Handle response
		int len = 0;
		if ( player->player_db != NULL && ( player->player_db->db->flags & AD_FLAG_BANNED ) ) {
			msg[0] = AD_ERR_BAN;
			len = 1;
			brk = 1;
		} else {
			brk = player_handler(msg, player, &len);
			if ( player->player_db != NULL && ( player->player_db->todo->len || player->player_db->db->task != AD_T_IDLE ) && msg[0] != AD_MULTI  ) {
				len++;
				int i;
				unsigned char a = '\0', b = '\0';
				for ( i = 0; i < len; i++ ) {
					b = msg[i];
					msg[i] = a;
					a = b;
				}
				msg[0] = AD_MULTI;
			}
		}
		// Send message to client
		ret = send(new_sck, (char *)msg, len, 0);
		if ( ret <= 0 ) {
			fprintf(log, "Connection lost to player %i during send.\n", player->id);
			break;
		}
	}
	player->player_db = NULL;
	shutdown(new_sck, SHUT_RDWR);
	close(new_sck);
	fprintf(log, "Player %i disconnected.\n", player->id); 
	player->id = 0;
	return NULL;
}

// Thread to handle connections
void *thread_socket() {
	struct sockaddr_in cli_addr;
	socklen_t cli_l = sizeof(cli_addr);
	while ( !term ) {
		int i = 0;
		while ( i < max_players ) {
			if ( players[i].id == 0 ) {
				break;
			}
			i++;
		}
		if ( i == max_players ) {
			printf("Too many players. Server needs to be restarted.\n");
			fprintf(log, "Too many players. Server needs to be restarted.\n");
			term = 1;
			break;
		}
		players[i].socket = accept(sck, (struct sockaddr *)&cli_addr, &cli_l);
		if ( players[i].socket >= 0 ) {
			players[i].state = AD_STATE_INIT;
			players[i].retry = 0;
			players[i].player_db = NULL;
			players[i].id = i + 1;
			int ret = pthread_create(&(players[i].thr), NULL, &thread_player, (void *)i);
			if ( ret != 0 ) {
				players[i].id = 0;
				shutdown(players->socket, SHUT_RDWR);
				close(players->socket);
			}
		}
	}
	return NULL;
}

int main(int argc, char **argv) {
	printf("Adventure server %d.%d.%d\n", ver_maj, ver_min, ver_rev);
	printf("by L Szabi - 2013\n\n");
	log = fopen("server.log", "a");
	time_t t_raw;
	time(&t_raw);
	fprintf(log, "Adventure server %d.%d.%d; %s", ver_maj, ver_min, ver_rev, ctime(&t_raw));
	int ret = 1;
	while ( ret ) {
		ret = start_db(0);
		if ( ret != 0 ) {
			printf("Fatal database error: %i\n", ret);
			fprintf(log, "Database error %i.\n", ret);
			return 1;
		}
	}
	printf("%d registered players.\n", players_n);
	fprintf(log, "%d registered players.\n", players_n);
	max_players = players_n + REG_LIMIT;
	ret = sizeof(player_t) * max_players;
	players = (player_t *)malloc(ret);
	memset(players, 0, ret);
	#ifdef WIN32
		WSADATA wsa;
		ret = WSAStartup(MAKEWORD(2, 2), &wsa);
		if ( ret != NO_ERROR ) {
			printf("WSA init failed.\n");
			fprintf(log, "WSA init failed.\n");
			WSACleanup();
			return 1;
		}
	#endif
	// I will replace it as soon as mingw starts supporting sigaction()
	signal(SIGINT, signal_close);
	signal(SIGTERM, signal_close);
	sck = socket(AF_INET, SOCK_STREAM, 0);
	if ( sck < 0 ) {
		printf("Socket init failed.\n");
		fprintf(log, "Socket init failed.\n");
		WSA_CLEAN();
		return 1;
	}
	int port = AD_PORT;
	if ( argc > 1 ) {
		port = toint(argv[1]);
	}
	if ( port == -1 ) {
		printf("Invalid port number.\n");
		close(sck);
		WSA_CLEAN();
		return 1;
	}
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	printf("Setting up server on port %d.\n", port);
	fprintf(log, "Using port %d.\n", port);
	ret = bind(sck, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if ( ret < 0 ) {
		printf("Socket bind failed.\n");
		fprintf(log, "Socket bind failed, error %i.\n", ret);
		close(sck);
		WSA_CLEAN();
		return 1;
	}
	listen(sck, 5);
	// Sockets set up
#ifndef NOADMIN
	pthread_t admin_thread;
	ret = pthread_create(&admin_thread, NULL, &thread_admin, NULL);
	if ( ret != 0 ) {
		printf("Could not create admin thread. Shutting down.\n");
		fprintf(log, "Could not create admin thread, error %i\n", ret);
		close(sck);
		WSA_CLEAN();
		return 1;
	}
#endif
	pthread_t socket_thread;
	ret = pthread_create(&socket_thread, NULL, &thread_socket, NULL);
	if ( ret != 0 ) {
		printf("Could not create socket thread. Shutting down.\n");
		fprintf(log, "Could not create socket thread, error %i.\n", ret);
		close(sck);
		WSA_CLEAN();
		return 1;
	}
	printf("Server initialized.\n");
	// Main loop
	while ( 1 ) {
		int i;
		for ( i = 0; i < players_n; i++ ) {
			user_t *cur = get_user(i);
			switch ( cur->db->task ) {
				case AD_T_IDLE:
				break;
				case AD_T_TRAVEL:
					;
					int arg = cur->db->task_arg;
					double dist = get_dist(cur->db->x, cur->db->y, ( arg & 0xFFFF0000 ) >> 16, arg & 0xFFFF);
					// t = s / v
					int t = (int)( ( dist / cur->db->mv_spd ) * 3600 );
					int time_rem = cur->db->task_started + t - time(NULL);
					if ( time_rem <= 0 ) {
						cur->db->task = AD_T_IDLE;
						time_rem = 0;
						cur->db->x = ( arg & 0xFFFF0000 ) >> 16;
						cur->db->y = arg & 0xFFFF;
					}
					cur->time_rem = time_rem;
				break;
			}
		}
		// Check if we should shut down
		if ( term ) {
			break;
		}
		do {
			sleep(AD_MAIN_DELAY);
		} while ( paused && !term );
	}
	// Terminate server
	printf("Shutting down.\n");
	unsigned char msg[32];
	sprintf((char *)msg, "%cServer is shutting down.", AD_MSG);
	bcast_msg(msg, strlen((char *)msg) + 1);
	msg[0] = AD_CLOSE;
	bcast_msg(msg, 1);
	sleep(3);
	close(sck);
	WSA_CLEAN();
	sleep(1);
	free(players);
	close_db(0);
	time(&t_raw);
	fprintf(log, "Will now quit: %s\n", ctime(&t_raw));
	fclose(log);
	printf("Will now quit.\n");
	return 0;
}
