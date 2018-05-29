#include "socket.h"
#include "console.h"
#include "common.h"
#include "database.h"
#include "game.h"
#include <signal.h>
#include <math.h>

// Player stages
#define STAGE_VERSION 0
#define STAGE_AUTH 1
#define STAGE_MAIN 2

static FILE *log_file;
static int socket_id = -1;

static int main_loop = 1;
static int main_pause = 0;

static time_t last_backup = 0;

int unexpected_packet(packet_t *packet) {
	int type = packet->type;
	packet->type = AD_PACKET_ERROR;
	packet->error.code = AD_ERROR_PROTOCOL;
	packet->error.flags = AD_ERROR_FLAG_MSG | AD_ERROR_FLAG_CLOSE;
	sprintf(packet->error.msg, "Unexpected packet type %x\n", type);
	return 1;
}

void stop_server() {
	main_loop = 0;
	packet_t packet;
	error_packet(&packet, AD_ERROR_SHUTDOWN, AD_ERROR_FLAG_MSG | AD_ERROR_FLAG_CLOSE, CL_FRONT_RED "Server is shutting down.");
	player_broadcast(&packet);
}

void signal_close2(int n) {
	signal(SIGINT, signal_close2);
	signal(SIGTERM, signal_close2);
	cprintf("Patience, please...\n");
}

void signal_close(int n) {
	signal(SIGINT, signal_close2);
	signal(SIGTERM, signal_close2);
	end_input();
	cprintf("%sClose signal detected.\n", CL_FRONT_BLUE);
	fprintf(log_file, "Close signal detected.\n");
	stop_server();
}

int process_input(char *input, char *out, char *name) {
	*out = '\0';
	if ( strlen(input) > 0 ) {
		// Explode string
		char *cmd = input;
		char *sub = str_explode_next(cmd, ' ');
		if ( char_eq(cmd, "close") || char_eq(cmd, "stop") ) {
			// Stop server
			cprintf("%sShut down by %s.\n", CL_FRONT_BLUE, name);
			fprintf(log_file, "Shut down by %s.\n", name);
			stop_server();
			return 1;
		} else if ( char_eq(cmd, "help") ) {
			// Print help
			if ( sub && char_eq(sub, "player") ) {
				sprintf(out, " %-32s - view player profile\n"
					" %-32s - ban/unban player\n"
					" %-32s - give/take player RCON permission\n"
					" %-32s - teleport player to x;y\n"
					" %-32s - send message 'str' to player",
					"player {name} profile", "player {name} ban", "player {name} rcon", "player {name} teleport {x} {y}", "player {name} send {str}"
				);
			} else if ( sub && char_eq(sub, "list") ) {
				sprintf(out, " %-32s - list players with RCON permission\n"
					" %-32s - list banned players",
					"list rcon", "list banned"
				);
			} else {
				sprintf(out, " %-32s - stop server\n"
					" %-32s - pause server\n"
					" %-32s - show player command help\n"
					" %-32s - show list command help\n"
					" %-32s - list from database name starting with str\n"
					" %-32s - print msg to server console\n"
					" %-32s - broadcast msg",
					"stop/close", "pause", "help player", "help list", "search {database} {str}", "print {msg}", "broadcast {msg}"
				);
			}
		} else if ( char_eq(cmd, "pause") ) {
			// Pause server
			main_pause = !main_pause;
			sprintf(out, "%sServer %spaused.", CL_FRONT_GREEN, main_pause ? "" : "un");
			if ( !char_eq(name, "admin") ) {
				cprintf("%sServer %spaused by %s.", CL_FRONT_BLUE, main_pause ? "" : "un", name);
			}
			fprintf(log_file, "Server %spaused by %s.\n", main_pause ? "" : "un", name);
		} else if ( char_eq(cmd, "player") && sub ) {
			// Player commands
			char *player_name = sub;
			sub = str_explode_next(player_name, ' ');
			char *arg = str_explode_next(sub, ' ');
			int idx = get_player(player_name);
			if ( idx == -1 ) {
				sprintf(out, "%sPlayer not found.", CL_FRONT_RED);
				return 0;
			}
			if ( char_eq(sub, "profile") ) {
				// Print profile
				packet_player_t *p = (packet_player_t *)&player_database[idx].name;
				if ( char_eq(name, "admin") ) {
					print_player_profile(p);
				} else {
					*out = 0x01;
					packet_t *packet = (packet_t *)(out + 1);
					packet->type = AD_PACKET_PLAYER;
					memcpy(&packet->player, p, sizeof(packet_player_t));
				}
			} else if ( char_eq(sub, "rcon") ) {
				// Toggle rcon flag
				player_database[idx].flags ^= AD_PLAYER_FLAG_RCON;
				int r = player_database[idx].flags & AD_PLAYER_FLAG_RCON;
				sprintf(out, "%s%s now %s RCON permission.", CL_FRONT_GREEN, player_database[idx].name, r ? "has" : "does not have");
			} else if ( char_eq(sub, "ban") ) {
				// Toggle ban flag
				player_database[idx].flags ^= AD_PLAYER_FLAG_BANNED;
				int r = player_database[idx].flags & AD_PLAYER_FLAG_BANNED;
				sprintf(out, "%s%s was %sbanned.", CL_FRONT_GREEN, player_database[idx].name, r ? "" : "un");
				packet_t packet;
				error_packet(&packet, AD_ERROR_BANNED, AD_ERROR_FLAG_MSG | AD_ERROR_FLAG_CLOSE, "You are banned.");
				player_send_im(idx, &packet);
			} else if ( char_eq(sub, "teleport") && arg ) {
				// Teleport player
				int err = 0;
				char *y_str = str_explode_next(arg, ' ');
				short x = tosint(arg, &err);
				short y = tosint(y_str, &err);
				if ( err || player_move(idx, x, y) == 1 ) {
					sprintf(out, "%sInvalid arguments.", CL_FRONT_RED);
				} else {
					sprintf(out, "%sTeleported %s to %d;%d.", CL_FRONT_GREEN, player_database[idx].name, x, y);
					packet_t packet;
					error_packet(&packet, AD_ERROR_OK, AD_ERROR_FLAG_MSG, NULL);
					sprintf(packet.error.msg, "%sYou were teleported to %d;%d by %s.", CL_FRONT_BLUE, x, y, name);
					player_send(idx, &packet);
				}
			} else if ( char_eq(sub, "send") && arg ) {
				// Send message
				player_send_msg(idx, name, arg);
				sprintf(out, "%sMessage sent.", CL_FRONT_GREEN);
			} else {
				sprintf(out, "Type 'help player' for help.");
			}
		} else if ( char_eq(cmd, "list") && sub ) {
			// List players
			if ( char_eq(sub, "rcon") ) {
				for ( int i = 0; i < player_database_n; i++ ) {
					if ( player_database[i].flags & AD_PLAYER_FLAG_RCON ) {
						sprintf(out + strlen(out), "%s, ", player_database[i].name);
					}
				}
			} else if ( char_eq(sub, "banned") ) {
				for ( int i = 0; i < player_database_n; i++ ) {
					if ( player_database[i].flags & AD_PLAYER_FLAG_BANNED ) {
						sprintf(out + strlen(out), "%s, ", player_database[i].name);
					}
				}
			} else {
				sprintf(out, "Type 'help list' for help.");
			}
		} else if ( char_eq(cmd, "search") && sub ) {
			char *start = str_explode_next(sub, ' ');
			// Search players
			if ( char_eq(sub, "players") ) {
				for ( int i = 0; i < player_database_n; i++ ) {
					if ( strncasecmp(player_database[i].name, start, strlen(start) ) == 0 ) {
						sprintf(out + strlen(out), "%s, ", player_database[i].name);
					}
				}
			} else {
				sprintf(out, "Databases: players");
			}
		} else if ( char_eq(cmd, "print") && sub ) {
			cprintf("[%s%s%s] %s\n", CL_FRONT_CYAN, name, CL_FRONT_DEFAULT, sub);
			sprintf(out, "%sMessage sent.", CL_FRONT_GREEN);
		} else if ( char_eq(cmd, "broadcast") && sub ) {
			packet_t packet;
			error_packet(&packet, AD_ERROR_OK, AD_ERROR_FLAG_MSG, NULL);
			snprintf(packet.error.msg, 128, "[%s%s%s] %s", CL_FRONT_CYAN, name, CL_FRONT_DEFAULT, sub);
			player_broadcast(&packet);
			sprintf(out, "%sMessage sent.", CL_FRONT_GREEN);
		} else {
			sprintf(out, "Type 'help' for help.");
		}
	}
	return 0;
}

int process_response(player_info_t *player, packet_t *packet) {
	if ( packet->type == AD_PACKET_CLOSE ) {
		return 1;
	}
	if ( packet->type == AD_PACKET_UNEXP ) {
		fprintf(log_file, "Sent 0x%x packet in stage %d.\n", packet->cmd.cmd, player->stage);
		packet->type = AD_PACKET_CLOSE;
		return 1;
	}
	if ( player->player_db != -1 && player_database[player->player_db].flags & AD_PLAYER_FLAG_BANNED ) {
		error_packet(packet, AD_ERROR_BANNED, AD_ERROR_FLAG_MSG | AD_ERROR_FLAG_CLOSE, "You are banned.");
		return 1;
	}
	if ( player->stage == STAGE_VERSION ) {
		// Init player
		if ( packet->type != AD_PACKET_VERSION ) {
			return unexpected_packet(packet);
		}
		if ( packet->version.major == AD_VER_MAJ && packet->version.minor == AD_VER_MIN && packet->version.revision == AD_VER_REV ) {
			packet_ok(packet);
			player->stage = STAGE_AUTH;
		} else {
			error_packet(packet, AD_ERROR_VERSION, AD_ERROR_FLAG_MSG | AD_ERROR_FLAG_CLOSE, NULL);
			sprintf(packet->error.msg, "%sVersion mismatch, you need version %d.%d.%d.", CL_FRONT_RED, AD_VER_MAJ, AD_VER_MIN, AD_VER_REV);
			return 1;
		}
	} else if ( player->stage == STAGE_AUTH ) {
		// Auth player
		if ( packet->type != AD_PACKET_AUTH ) {
			return unexpected_packet(packet);
		}
		int db_idx = get_player(packet->auth.player_name);
		if ( packet->auth.new_player ) {
			if ( db_idx == -1 ) {
				// Register
				int ret = player_create(packet->auth.player_name, packet->auth.hash);
				if ( ret == 1 || ret == -1 ) {
					error_packet(packet, AD_ERROR_REGISTER, AD_ERROR_FLAG_MSG, CL_FRONT_RED "Couldn't create player.");
				} else {
					last_backup = get_time();
					db_idx = player_database_n - 1;
					error_packet(packet, AD_ERROR_OK, AD_ERROR_FLAG_MSG, NULL);
					sprintf(packet->error.msg, "%sWelcome, %s! Your adventure is about to begin...", CL_FRONT_GREEN, player_database[db_idx].name);
					player->player_db = db_idx;
					player->stage = STAGE_MAIN;
				}
				if ( ret == -1 ) {
					cprintf("%sCouldn't create player: database backup failed.\n", CL_FRONT_RED);
					fprintf(log_file, "Couldn't create player: database backup failed.\n");
				}
			} else {
				error_packet(packet, AD_ERROR_REGISTER, AD_ERROR_FLAG_MSG, CL_FRONT_RED "This name is taken.");
			}
		} else {
			if ( db_idx == -1 ) {
				error_packet(packet, AD_ERROR_LOGIN, AD_ERROR_FLAG_MSG, CL_FRONT_RED "User not found.");
			} else {
				// Log in
				if ( player_database[db_idx].flags & AD_PLAYER_FLAG_BANNED ) {
					error_packet(packet, AD_ERROR_BANNED, AD_ERROR_FLAG_MSG | AD_ERROR_FLAG_CLOSE, "You are banned.");
					return 1;
				}
				if ( memcmp(packet->auth.hash, player_database[db_idx].password_hash, sizeof(char) * 16) == 0 ) {
					error_packet(packet, AD_ERROR_OK, AD_ERROR_FLAG_MSG, NULL);
					sprintf(packet->error.msg, "%sWelcome back, %s!", CL_FRONT_GREEN, player_database[db_idx].name);
					player->player_db = db_idx;
					player->stage = STAGE_MAIN;
				} else {
					error_packet(packet, AD_ERROR_PASSWD, AD_ERROR_FLAG_MSG, CL_FRONT_RED "Incorrect password.");
				}
			}
		}
	} else if ( player->stage == STAGE_MAIN ) {
		if ( packet->type == AD_PACKET_CMD ) {
			if ( packet->cmd.cmd == AD_CMD_PLAYER ) {
				// Print profile
				packet->type = AD_PACKET_PLAYER;
				if ( packet->cmd.arg[0] == '\0' ) {
					memcpy(&(packet->player), &(player_database[player->player_db].name), sizeof(packet_player_t));
				} else {
					int idx = get_player(packet->cmd.arg);
					if ( idx != -1 && player_can_meet(idx, player->player_db) ) {
						memcpy(&(packet->player), &(player_database[idx].name), sizeof(packet_player_t));
					} else {
						error_packet(packet, AD_ERROR_SENDMSG, AD_ERROR_FLAG_MSG, CL_FRONT_RED "Failed to send message. ( Player not found or not in the same area )");
					}
				}
			} else if ( packet->cmd.cmd == AD_CMD_RCON ) {
				// Process rcon input
				if ( player_database[player->player_db].flags & AD_PLAYER_FLAG_RCON ) {
					char ret[2048];
					process_input(packet->cmd.arg, ret, player_database[player->player_db].name);
					if ( ret[0] == 1 ) {
						memcpy(packet, ret + 1, sizeof(packet_t));
					} else {
						player_send_fragments(player, ret, packet);
					}
				} else {
					error_packet(packet, AD_ERROR_RCON, AD_ERROR_FLAG_MSG, CL_FRONT_RED "You do not have RCON permission.");
				}
			} else if ( packet->cmd.cmd == AD_CMD_SENDMSG ) {
				char *name = packet->cmd.arg;
				char *msg = str_explode_next(name, ';');
				int idx = get_player(name);
				if ( idx != -1 && player_can_meet(idx, player->player_db) ) {
					player_send_msg(idx, player_database[player->player_db].name, msg);
					error_packet(packet, AD_ERROR_OK, AD_ERROR_FLAG_MSG, CL_FRONT_GREEN "Message sent.");
				} else {
					error_packet(packet, AD_ERROR_SENDMSG, AD_ERROR_FLAG_MSG, CL_FRONT_RED "Failed to send message. ( Player not found or not in the same area )");
				}
			}
		} else {
			return unexpected_packet(packet);
		}
	}
	return 0;
}

int leave() {
	socket_close(socket_id);
	WSA_CLEAN();
	cleanup_console();
	return 1;
}

int main(int argc, char *argv[]) {
	setup_console();
	cprintf("!CAdventure server %d.%d.%d\n\n", AD_VER_MAJ, AD_VER_MIN, AD_VER_REV);
	int server_port = AD_PORT;
	if ( argc > 1 ) {
		int n = toint(argv[1]);
		if ( n != -1 ) {
			server_port = n;
		} else {
			cprintf("%sInvalid port number, using default port %d\n", CL_FRONT_YELLOW, AD_PORT);
		}
	}
	// Start socket
	socket_id = socket_create();
	if ( socket_id == -1 ) {
		cprintf("%sFailed to create socket.\n", CL_FRONT_RED);
		return leave();
	}
	if ( socket_listen(socket_id, server_port) < 0 ) {
		cprintf("%sFailed to bind socket.\n", CL_FRONT_RED);
		return leave();
	}
	// Start database
	if ( database_open() < 0 ) {
		cprintf("%sFailed to open database.\n", CL_FRONT_RED);
		return leave();
	}
	if ( database_backup() < 0 ) {
		cprintf("%sFailed to backup database.\n", CL_FRONT_RED);
		return leave();
	}
	last_backup = get_time();
	log_file = fopen("server.log", "a");
	if ( log_file == NULL ) {
		cprintf("%sFailed to open server.log.\n", CL_FRONT_RED);
		return leave();
	}
	fprintf(log_file, "Adventure server %d.%d.%d started on %s\n", AD_VER_MAJ, AD_VER_MIN, AD_VER_REV, current_time());
	players = 0;
	player_info_t *prev_player = 0;
	get_input("admin> ");
	srand(get_time());
	signal(SIGINT, signal_close);
	signal(SIGTERM, signal_close);
	while ( 1 ) {
		if ( read_console() == 2 ) {
			char output[2048];
			char *input = end_input_echo();
			if ( input != NULL && strlen(input) > 0 ) {
				if ( process_input(input, output, "admin") == 0 ) {
					get_input("admin> ");
				}
				cprintf("%s\n", output);
			} else {
				get_input("admin> ");
			}
		}
		int new_socket = socket_accept(socket_id);
		if ( new_socket > 0 ) {
			player_info_t *new_player = (player_info_t *)malloc(sizeof(player_info_t));
			memset(new_player, 0, sizeof(player_info_t));
			new_player->socket = new_socket;
			new_player->player_db = -1;
			new_player->stage = STAGE_VERSION;
			new_player->next = 0;
			if ( players == 0 ) {
				new_player->prev = 0;
				players = new_player;
			} else {
				new_player->prev = prev_player;
				prev_player->next = new_player;
			}
		} else if ( new_socket < 0 ) {
			cprintf("%sAccept failed.\n", CL_FRONT_RED);
		}
		player_info_t *player = players;
		while ( player != 0 ) {
			// Receive from client
			packet_t packet;
			int received = socket_recv(player->socket, (char *)&packet, sizeof(packet_t));
			if ( received < 0 ) {
				player_close(player);
			} else if ( received > 0 ) {
				process_response(player, &packet); // check for dc
				// Send response
				player_socket_send(player, &packet);
			}
			// Check for messages
			if ( player->player_db != -1 ) {
				player_check_inbox(player->player_db);
			}
			prev_player = player;
			player = player->next;
		}
		// Shutdown
		if ( !main_loop && players == 0 ) {
			break;
		}
		// Backup
		if ( last_backup + BACKUP_TIME < get_time() ) {
			cprintf("%sBackup in progress...\n", CL_FRONT_GREEN);
			int r = database_backup();
			if ( r < 0 ) {
				cprintf("%sDatabase backup failed.", CL_FRONT_RED);
				fprintf(log_file, "Database backup failed.\n");
				stop_server();
			}
			last_backup = get_time();
		}
	}
	end_input();
	if ( database_close() < 0 ) {
		cprintf("%sFailed to close database.\n", CL_FRONT_RED);
		fprintf(log_file, "Failed to close database.\n");
	}
	fprintf(log_file, "Server closed on %s\n\n", current_time());
	fclose(log_file);
	cprintf("Will now quit.\n");
	leave();
	return 0;
}
