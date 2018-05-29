#include "socket.h"
#include "console.h"
#include "common.h"
#include "md5.h"

// Stage
#define STAGE_VERSION 0
#define STAGE_AUTH 1
#define STAGE_MAIN 2

static int client_stage = 0;

// Input
#define INPUT_NONE 0
#define INPUT_AUTH 1
#define INPUT_NAME 2
#define INPUT_PASSWORD 3
#define INPUT_MAIN 4
#define INPUT_RCON 5
#define INPUT_MSG_NAME 6
#define INPUT_MSG_STR 7
#define INPUT_PLAYER_NAME 8
#define INPUT_TRAVEL_X 9
#define INPUT_TRAVEL_Y 10
#define INPUT_ITEM_NAME 11
#define INPUT_ITEM_QTY 12

static int user_input = 0;
static packet_t temp_input_packet;

static char multi_packet[2048] = { '\0' };

static int unexpected_packet(packet_t *packet) {
	uint16_t t = packet->type;
	cprintf("%sUnexpected packet from server. (type: %#0.2x)\n", CL_FRONT_RED, t);
	packet->type = AD_PACKET_UNEXP;
	packet->cmd.cmd = t;
	return 2;
}

static int error_handler(packet_t *packet) {
	if ( packet->error.code == AD_ERROR_OK ) {
		if ( packet->error.flags & AD_ERROR_FLAG_MSG ) {
			cprintf("%s\n", packet->error.msg);
		}
	} else if ( packet->error.flags & AD_ERROR_FLAG_MSG ) {
		cprintf("%sError %#0.2x: %s\n", CL_FRONT_RED, packet->error.code, packet->error.msg);
	} else {
		cprintf("%sError %#0.2x\n", CL_FRONT_RED, packet->error.code);
	}
	if ( packet->error.flags & AD_ERROR_FLAG_CLOSE ) {
		return 1;
	}
	return 0;
}

static void auth_prompt() {
	cprintf("Welcome to Adventure! Please log in.\n");
	cprintf(" l - login\n");
	cprintf(" r - register\n");
	cprintf(" c - close\n");
	user_input = INPUT_AUTH;
	set_input(1);
}

static void main_prompt() {
	cprintf("What would you like to do? ");
	user_input = INPUT_MAIN;
	set_input(1);
}

/* Process server response
 * 0 - no reply
 * 1 - exit loop
 * 2 - reply
 */
static int process_response(packet_t *packet) {
	if ( packet->type == AD_PACKET_CLOSE ) {
		cprintf("%sConnection closed.\n", CL_FRONT_BLUE);
		return 1;
	}
	if ( client_stage == STAGE_VERSION ) {
		if ( packet->type != AD_PACKET_ERROR ) {
			return unexpected_packet(packet);
		}
		if ( packet->error.code == AD_ERROR_OK ) {
			auth_prompt();
			client_stage = STAGE_AUTH;
		} else {
			if ( error_handler(packet) ) {
				return 1;
			}
			packet->type = AD_PACKET_CLOSE;
			return 2;
		}
	} else if ( client_stage == STAGE_AUTH ) {
		if ( packet->type != AD_PACKET_ERROR ) {
			return unexpected_packet(packet);
		}
		if ( packet->error.code == AD_ERROR_OK ) {
			cprintf("%s\n", packet->error.msg);
			client_stage = STAGE_MAIN;
			main_prompt();
		} else {
			if ( error_handler(packet) ) {
				return 1;
			}
			auth_prompt();
		}
	} else if ( client_stage == STAGE_MAIN ) {
		if ( packet->type == AD_PACKET_PLAYER ) {
			print_player_profile(&packet->player);
		} else if ( packet->type == AD_PACKET_ERROR ) {
			if ( error_handler(packet) ) {
				return 1;
			}
		} else if ( packet->type == AD_PACKET_MULTI ) {
			sprintf(multi_packet + strlen(multi_packet), "%s", packet->multi.msg);
			if ( packet->multi.last ) {
				cprintf("%s\n", multi_packet);
				multi_packet[0] = '\0';
			}
		} else if ( packet->type == AD_PACKET_TASK ) {
			if ( user_input == INPUT_NONE || user_input == INPUT_MAIN ) {
				if ( packet->task.type == AD_TASK_TRAVEL ) {
					char buf[32];
					main_prompt();
					cprintf("You are on your way to %d;%d.!R%s", packet->task.travel.x, packet->task.travel.y, display_time(buf, packet->task.time_rem));
					return 0;
				}
			} else {
				return 0;
			}
		} else {
			return unexpected_packet(packet);
		}
		main_prompt();
	}
	return 0;
}

static int process_input(packet_t *packet) { // this should never return 1
	if ( user_input == 0 ) {
		end_input();
		set_input(0);
	} else if ( user_input == INPUT_AUTH ) {
		if ( read_console() == 1 ) {
			char c = get_char();
			if ( c == 'l' ) {
				temp_input_packet.auth.new_player = 0;
			} else if ( c == 'r' ) {
				temp_input_packet.auth.new_player = 1;
			} else if ( c == 'c' ) {
				set_input(0);
				packet->type = AD_PACKET_CLOSE;
				return 2;
			}
			if ( c == 'l' || c == 'r' ) {
				get_input("Name: ");
				temp_input_packet.type = AD_PACKET_AUTH;
				user_input = INPUT_NAME;
			}
		}
	} else if ( user_input == INPUT_NAME ) {
		if ( read_console() == 2 ) {
			char *name = end_input_echo();
			strncpy(temp_input_packet.auth.player_name, name, 16);
			get_password("Password: ");
			user_input = INPUT_PASSWORD;
		}
	} else if ( user_input == INPUT_PASSWORD ) {
		if ( read_console() == 2 ) {
			char *passwd = end_input_echo();
			set_input(0);
			md5((uint8_t *)passwd, strlen(passwd));
			memcpy(temp_input_packet.auth.hash, passwd, sizeof(char) * 16);
			memcpy(packet, &temp_input_packet, sizeof(packet_t));
			user_input = INPUT_NONE;;
			return 2;
		}
	} else if ( user_input == INPUT_MAIN ) {
		if ( read_console() == 1 ) {
			char c = get_char();
			if ( c == 'c' ) {
				set_input(0);
				packet->type = AD_PACKET_CLOSE;
				return 2;
			} else if ( c == 'p' ) {
				set_input(0);
				packet->type = AD_PACKET_CMD;
				packet->cmd.cmd = AD_CMD_PLAYER;
				packet->cmd.arg[0] = '\0';
				return 2;
			} else if ( c == 'i' ) {
				set_input(0);
				packet->type = AD_PACKET_CMD;
				packet->cmd.cmd = AD_CMD_INVERTORY;
				return 2;
			} else if ( c == 'o' ) {
				memset(&temp_input_packet, 0, sizeof(packet_t));
				temp_input_packet.type = AD_PACKET_CMD;
				temp_input_packet.cmd.cmd = AD_CMD_PLAYER;
				temp_input_packet.cmd.arg[0] = '\0';
				get_input("Player name: ");
				user_input = INPUT_PLAYER_NAME;
			} else if ( c == 'r' ) {
				get_input("rcon> ");
				user_input = INPUT_RCON;
			} else if ( c == 's' ) {
				get_input("Player name: ");
				user_input = INPUT_MSG_NAME;
			} else if ( c == 't' ) {
				get_input("X: ");
				user_input = INPUT_TRAVEL_X;
			} else if ( c == 'a' ) {
				packet->type = AD_PACKET_TASK;
				packet->task.type = AD_TASK_IDLE;
				return 2;
			} else if ( c == 'g' ) {
				memset(&temp_input_packet, 0, sizeof(packet_t));
				temp_input_packet.type = AD_PACKET_CMD;
				temp_input_packet.cmd.cmd = AD_CMD_GIVE;
				temp_input_packet.cmd.arg[0] = '\0';
				get_input("Item name: ");
				user_input = INPUT_ITEM_NAME;
			} else {
				cprintf(" t - travel\n");
				cprintf(" a - abort current task\n");
				cprintf(" i - list invertory\n");
				cprintf(" p - view profile\n");
				cprintf(" o - view other player's profile\n");
				cprintf(" s - send message to player\n");
				cprintf(" g - give item to player\n");
				cprintf(" c - close\n");
				main_prompt();
			}
		}
	} else if ( user_input == INPUT_RCON ) {
		if ( read_console() == 2 ) {
			char *cmd = end_input_echo();
			memset(packet, 0, sizeof(packet_t));
			packet->type = AD_PACKET_CMD;
			packet->cmd.cmd = AD_CMD_RCON;
			strncpy(packet->cmd.arg, cmd, 63);
			user_input = INPUT_NONE;;
			return 2;
		}
	} else if ( user_input == INPUT_MSG_NAME ) {
		if ( read_console() == 2 ) {
			char *name = end_input_echo();
			memset(&temp_input_packet, 0, sizeof(packet_t));
			temp_input_packet.type = AD_PACKET_CMD;
			temp_input_packet.cmd.cmd = AD_CMD_SENDMSG;
			strncpy(temp_input_packet.cmd.arg, name, 15);
			user_input = INPUT_MSG_STR;
			get_input("Message: ");
		}
	} else if ( user_input == INPUT_MSG_STR ) {
		if ( read_console() == 2 ) {
			char *msg = end_input_echo();
			char *c = temp_input_packet.cmd.arg;
			sprintf(c + strlen(c), ";");
			strncpy(c + strlen(c), msg, 127 - strlen(c));
			memcpy(packet, &temp_input_packet, sizeof(packet_t));
			user_input = INPUT_NONE;;
			return 2;
		}
	} else if ( user_input == INPUT_PLAYER_NAME ) {
		if ( read_console() == 2 ) {
			char *name = end_input_echo();
			strncpy(temp_input_packet.cmd.arg + strlen(temp_input_packet.cmd.arg), name, 15);
			user_input = INPUT_NONE;
			memcpy(packet, &temp_input_packet, sizeof(packet_t));
			return 2;
		}
	} else if ( user_input == INPUT_TRAVEL_X ) {
		if ( read_console() == 2 ) {
			char *x_str = end_input();
			int e = 0;
			short x = tosint(x_str, &e);
			if ( e || !RANGE(x, -500, 500) ) {
				cprintf("Invalid coordinate!\n");
				user_input = INPUT_MAIN;
			} else {
				temp_input_packet.type = AD_PACKET_TASK;
				temp_input_packet.task.type = AD_TASK_TRAVEL;
				temp_input_packet.task.travel.x = x;
				user_input = INPUT_TRAVEL_Y;
				char p[32];
				sprintf(p, "X: %s Y: ", x_str);
				get_input(p);
			}
		}
	} else if ( user_input == INPUT_TRAVEL_Y ) {
		if ( read_console() == 2 ) {
			char *y_str = end_input_echo();
			int e = 0;
			short y = tosint(y_str, &e);
			if ( e || !RANGE(y, -500, 500) ) {
				cprintf("Invalid coordinate!\n");
				user_input = INPUT_MAIN;
			} else {
				temp_input_packet.task.travel.y = y;
				memcpy(packet, &temp_input_packet, sizeof(packet_t));
				user_input = INPUT_NONE;
				return 2;
			}
		}
	} else if ( user_input == INPUT_ITEM_NAME ) {
		if ( read_console() == 2 ) {
			sprintf(temp_input_packet.cmd.arg, "%s;", end_input_echo());
			get_input("Item quantity: ");
			user_input = INPUT_ITEM_QTY;
		}
	} else if ( user_input == INPUT_ITEM_QTY ) {
		if ( read_console() == 2 ) {
			int e = toint(end_input_echo());
			if ( e > 0 ) {
				temp_input_packet.cmd.arg_n[0] = e;
				get_input("Player name: ");
				user_input = INPUT_PLAYER_NAME;
			} else {
				cprintf("Invalid number!\n");
				user_input = INPUT_MAIN;
			}
		}
	}
	return 0;
}

int main(int argc, char *argv[]) {
	setup_console();
	cprintf("!CAdventure client %d.%d.%d\n\n", AD_VER_MAJ, AD_VER_MIN, AD_VER_REV);
	int port = AD_PORT;
	if ( argc < 2 ) {
		cprintf("Usage: client ip-address [port]\n", CL_FRONT_YELLOW);
		cleanup_console();
		return 1;
	} else if ( argc > 2 ) {
		int n = toint(argv[2]);
		if ( n != -1 ) {
			port = n;
		} else {
			cprintf("%sInvalid port number, using default port %d\n", CL_FRONT_YELLOW, AD_PORT);
		}
	}
	// Setting up socket
	int socket_id = socket_create();
	if ( socket_id < 0 ) {
		cprintf("%sFailed to create socket.\n", CL_FRONT_RED);
		cleanup_console();
		return 1;
	}
	if ( socket_connect(socket_id, argv[1], port) < 0 ) {
		cprintf("%sFailed to connect to server.\n", CL_FRONT_RED);
		socket_close(socket_id);
		cleanup_console();
		return 1;
	}
	int reply = 2;
	packet_t packet;
	packet.type = AD_PACKET_VERSION;
	packet.version.major = AD_VER_MAJ;
	packet.version.minor = AD_VER_MIN;
	packet.version.revision = AD_VER_REV;
	while ( 1 ) {
		if ( reply == 0 ) {
			reply = process_input(&packet);
		}
		if ( reply == 2 ) {
			reply = 0;
			if ( socket_send(socket_id, (char *)&packet, sizeof(packet_t)) < 0 ) {
				cprintf("%sFailed to send data.\n", CL_FRONT_RED);
				break;
			}
		}
		int received = socket_recv(socket_id, (char *)&packet, sizeof(packet_t));
		if ( received < 0 ) {
			cprintf("%sFailed to receive data. (Connection interrupted)\n", CL_FRONT_RED);
			break;
		} else if ( received > 0 ) {
			reply = process_response(&packet);
			if ( reply == 1 ) {
				break;
			}
		}
	}
	socket_close(socket_id);
	WSA_CLEAN();
	cprintf("Bye!\n");
	cleanup_console();
	return 0;
}