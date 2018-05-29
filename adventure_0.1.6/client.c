#include "common.h"
#include "md5.h"
#ifndef WIN32
	#include <termios.h>
#endif

/*

	Adventure client
	by L Szabi 2013
	
*/

const int ver_maj = 0, ver_min = 1, ver_rev = 6;

static int state = AD_STATE_INIT, abrt = 0;

// Set console echo
#ifdef WIN32
static void echo(int on) {
	DWORD mode;
	HANDLE hConIn = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(hConIn, &mode);
	mode = on ? ( mode | ENABLE_ECHO_INPUT ) : ( mode & ~( ENABLE_ECHO_INPUT ) );
	SetConsoleMode(hConIn, mode);
}
#else
static void echo(int on) {
	struct termios settings;
	tcgetattr(STDIN_FILENO, &settings);
	settings.c_lflag = on ? ( settings.c_lflag | ECHO ) : ( settings.c_lflag & ~( ECHO ) );
	tcsetattr(STDIN_FILENO, TCSANOW, &settings);
}
#endif

// Handle close signal sent by os, use it as abort ( Crtl-C )
void signal_handler(int n) {
	signal(SIGINT, &signal_handler);
	abrt = 1;
}

// Parse input from stdin
static int parse_usr_command(unsigned char cmd, unsigned char *msg) {
	char buf[256];
	switch ( cmd ) {
		case AD_AUTH:
			while ( 1 ) {
				printf("Authentication required.\n 1 - Login\n 2 - Register\n 3 - Exit\n");
				get_input("", buf);
				char c = buf[0];
				int i = 0;
				if ( c == '1' || c == '2' ) {
					i = get_input_l("Name: ", buf, AD_BUF - 1);
					strcpy((char *)(msg + 1), buf);
				}
				switch ( c ) {
					case '1':
						msg[0] = AD_LOGIN;
						state = AD_STATE_LOGIN;
						return i + 1;
					break;
					case '2':
						msg[0] = AD_REG;
						state = AD_STATE_REG;
						return i + 1;
					break;
					case '3':
						msg[0] = AD_LOGOUT;
						return 1;
					break;
				}
			}
		break;
		case AD_PASSW:
			echo(0);
			get_input("Password: ", buf);
			echo(1);
			printf("[******]\n");
			md5_hash((uint8_t *)buf, msg);
			return 16;
		break;
		case AD_STATE_MAIN:
		case AD_ABORT:
			while ( 1 ) {
				printf("What do you want to do now?\n");
				if ( cmd == AD_ABORT ) {
					printf(" c - Abort task\n");
				}
				printf(" n - Nothing\n");
				printf(" p - Get position\n");
				if ( cmd != AD_ABORT ) {
					printf(" t - Travel!\n");
				}
				printf(" s - Send message to player\n");
				printf(" l - List players in this area\n");
				printf(" a - Account/profile\n");
				printf(" q - Exit\n");
				get_input("", buf);
				switch ( buf[0] ) {
					case 'a':
						while ( 1 ) {
							printf("My account\n");
							printf(" p - View profile\n");
							printf(" c - Change password\n");
							printf(" q - Cancel\n");
							get_input("", buf);
							if ( buf[0] == 'q' ) {
								break;
							}
							switch ( buf[0] ) {
								case 'p':
									msg[0] = AD_S_PROF;
									return 1;
								break;
								case 'c':
									return parse_usr_command(AD_PASSW, msg);
								break;
							}
						}
					break;
					case 'q':
						msg[0] = AD_LOGOUT;
						return 1;
					break;
					case 'n':
						msg[0] = AD_UPD;
						return 1;
					break;
					case 'p':
						msg[0] = AD_S_POS;
						return 1;
					break;
					case 's':
						msg[0] = AD_SENDMSG;
						int l = get_input_l("Player: ", buf, 16) + 1;
						l += get_input_l("Message: ", buf + l, AD_BUF - 1 - l) + 1;
						memcpy(msg + 1, buf, l);
						return l + 1;
					break;
					case 't':
						if ( cmd != AD_ABORT ) {
							msg[0] = AD_TASK;
							msg[1] = AD_T_TRAVEL;
							short x = 0, y = 0;
							do {
								get_input("Enter coordinate x: ", buf);
								int err = 0;
								x = tosint(buf, &err);
								if ( err ) {
									printf("Invalid number.\n");
									continue;
								}
								if ( x > 500 || x < -500 ) {
									printf("Invalid coordinates.\n");
									continue;
								}
							} while ( 0 );
							do {
								get_input("Enter coordinate y: ", buf);
								int err = 0;
								y = tosint(buf, &err);
								if ( err ) {
									printf("Invalid number.\n");
									continue;
								}
								if ( y > 500 || y < -500 ) {
									printf("Invalid coordinates.\n");
									continue;
								}
							} while ( 0 );
							print_short(msg + 2, x);
							print_short(msg + 4, y);
							return 6;
						}
					break;
					case 'l':
						printf("Players here:\n");
						msg[0] = AD_GETPLIST;
						return 1;
					break;
					case 'c':
						if ( cmd == AD_ABORT ) {
							msg[0] = AD_ABORT;
							return 1;
						}
					break;
					case 'r':
						msg[0] = AD_RCON_MSG;
						return get_input("RCON message: ", (char *)( msg + 1 )) + 2;
					break;
				}
			}
		break;
	}
	return 0;
}

// Parse packets received from server
static int parse_server(unsigned char *msg, int *len, int multi) {
	printf("\r                                                                               \r"); // Needed for line update
	switch ( state ) {
		case AD_STATE_INIT:
			switch ( msg[0] ) {
				case AD_MAGIC_SERVER:
					msg[0] = AD_MAGIC_CLIENT;
					state = AD_STATE_VER;
					*len = 1;
				break;
				default:
					printf("Invalid server magic.\n");
					msg[0] = AD_LOGOUT;
					state = AD_STATE_VER;
					*len = 1;
				break;
			}
		break;
		case AD_STATE_VER:
			switch ( msg[0] ) {
				case AD_VER:
					print_int(msg, ver_maj);
					print_int(msg + 4, ver_min);
					print_int(msg + 8, ver_rev);
					*len = 12;
				break;
				case AD_ERR_MAGIC:
					printf("Invalid client magic id.\n");
					return 1;
				break;
				case AD_ERR_VER:
					printf("Server version is %d.%d.%d. Please get the latest update.\n", get_int(msg + 1), get_int(msg + 5), get_int(msg + 9));
					return 1;
				break;
				case AD_ACK:
					msg[0] = AD_OK;
					*len = 1;
				break;
				case AD_AUTH:
					*len = parse_usr_command(AD_AUTH, msg);
				break;
				case AD_ERR_COMM:
					printf("Seems like I've sent 0x%x in AD_STATE_VER. Sorry.\n", (int)( msg[1] )); // no break
				case AD_CLOSE:
					return 1;
				break;
				default:
					printf("Communication protocol error: received 0x%x in AD_STATE_VER.\n", (int)( msg[0] ));
					msg[1] = msg[0];
					msg[0] = AD_ERR_COMM;
					*len = 2;
				break;
			}
		break;
		case AD_STATE_REG:
			switch ( msg[0] ) {
				case AD_PASSW:
					msg[0] = AD_CHPASSW;
					*len = parse_usr_command(AD_PASSW, msg + 1) + 1;
				break;
				case AD_ERR_UNAME:
					printf("Invalid or reserved username.\n");
					*len = parse_usr_command(AD_AUTH, msg);
				break;
				case AD_ACK:
					printf("Welcome! Let your adventure begin!\n");
					msg[0] = AD_S_POS;
					*len = 1;
					state = AD_STATE_MAIN;
				break;
				case AD_ERR_COMM:
					printf("Seems like I've sent 0x%x in AD_STATE_REG. Sorry.\n", (int)( msg[1] )); // no break
				case AD_CLOSE:
					return 1;
				break;
				case AD_ERR:
					printf("Internal server error occured.\n");
					return 1;
				break;
				case AD_ERR_BAN: // do we really ned this here?
					printf("Oops... Looks like you... are... BANNED!!!\n");
					return 1;
				break;
				default:
					printf("Communication protocol error: received 0x%x in AD_STATE_REG.\n", (int)( msg[0] ));
					msg[1] = msg[0];
					msg[0] = AD_ERR_COMM;
					*len = 2;
				break;
			}
		break;
		case AD_STATE_LOGIN:
			switch ( msg[0] ) {
				case AD_ERR_PASSW:
					printf("Incorrect password.\n");
					*len = parse_usr_command(AD_AUTH, msg);
				break;
				case AD_PASSW:
					msg[0] = AD_ENTPASSW;
					*len = parse_usr_command(AD_PASSW, msg + 1) + 1;
				break;
				case AD_ERR_UNAME:
					printf("Unknown username.\n");
					*len = parse_usr_command(AD_AUTH, msg);
				break;
				case AD_ACK:
					printf("Welcome!\n");
					msg[0] = AD_S_POS;
					*len = 1;
					state = AD_STATE_MAIN;
				break;
				case AD_ERR_COMM:
					printf("Seems like I've sent 0x%x in AD_STATE_LOGIN. Sorry.\n", (int)( msg[1] )); // no break
				case AD_CLOSE:
					return 1;
				break;
				case AD_ERR_BAN:
					printf("Oops... Looks like you... are... BANNED!!!\n");
					return 1;
				break;
				default:
					printf("Communication protocol error: received 0x%x in AD_STATE_LOGIN.\n", (int)( msg[0] ));
					msg[1] = msg[0];
					msg[0] = AD_ERR_COMM;
					*len = 1;
				break;
			}
		break;
		case AD_STATE_MAIN:
			switch ( msg[0] ) {
				case AD_ERR_COMM:	
					printf("Seems like I've sent 0x%x in AD_STATE_VER. Sorry.\n", (int)( msg[1] )); // no break
				case AD_CLOSE:
					return 1;
				break;
				case AD_ACK:
					printf("Operation succeeded.\n");
					*len = 0;
				break;
				case AD_NOP:
					printf("Nothing new.\n");
					*len = parse_usr_command(AD_STATE_MAIN, msg);
				break;
				case AD_MSG:
					printf("%s\n", (char *)(msg + 1));
					*len = 0;
				break;
				case AD_S_POS:
					printf("Your current position is %i;%i.\n", get_short(msg + 1), get_short(msg + 3));
					*len = 0;
				break;
				case AD_BUSY:
					if ( abrt ) {
						abrt = 0;
						*len = parse_usr_command(AD_ABORT, msg);
					} else {
						switch ( msg[1] ) {
							case AD_T_TRAVEL:
								printf("You are en route for %s...", disp_time((char *)(msg + 10), get_int(msg + 2)));
								msg[0] = AD_UPD;
								*len = 1;
							break;
						}
					}
				break;
				case AD_S_PROF:
					printf("\nYour character:\n\n");
					printf("Level %i, %i xp\n\n", get_int(msg + 1), get_int(msg + 5));
					printf("ATK\t%i\tDEF\t%i\n", get_int(msg + 9), get_int(msg + 13));
					printf("SPD\t%i\tDMG\t%i\n", get_int(msg + 17), get_int(msg + 21));
					printf("HP\t%i\tMAX HP\t%i\n", get_int(msg + 25), get_int(msg + 29));
					printf("Movement speed: %i\n", get_int(msg + 33));
					if ( get_int(msg + 37) & AD_FLAG_RCON ) {
						printf("You are an RCON admin.\n");
					}
					printf("\n");
					*len = 0;
				break;
				case AD_PLIST:
					{
						int i = 1, f = 0;
						while ( 1 ) {
							printf("%s", (char *)msg + i);
							int j = strlen((char *)msg + i) + 1;
							i += j--;
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
							if ( i == AD_BUF || *(msg + i) == '\0' ) {
								break;
							}
						}
						if ( f != 0 ) {
							printf("\n");
						}
					}
					*len = 0;
				break;
				case AD_ERR_BAN:
					printf("Oops... Looks like you... are... BANNED!!!\n");
					return 1;
				break;
				default:
					printf("Communication protocol error: received 0x%x in AD_STATE_MAIN.\n", (int)( msg[0] ));
					msg[1] = msg[0];
					msg[0] = AD_ERR_COMM;
					*len = 2;
				break;
			}
			// Check multi here
			if ( *len == 0 ) {
				if ( multi ) {
					msg[0] = AD_UPD;
					*len = 1;
				} else {
					*len = parse_usr_command(AD_STATE_MAIN, msg);
				}
			}
		break;
	}
	return 0;
}

int main(int argc, char **argv) {
	printf("Adventure client %d.%d.%d\n", ver_maj, ver_min, ver_rev);
	printf("by L Szabi - 2013\n\n");
	if ( argc < 2 ) {
		printf("Usage: client server_ip [port]\n");
		return 0;
	}
	int port = AD_PORT;
	if ( argc > 2 ) {
		port = toint(argv[2]);
	}
	if ( port == -1 ) {
		printf("Invalid port number.\n");
		return 1;
	}
	#ifdef WIN32
		WSADATA wsa;
		int res = WSAStartup(MAKEWORD(2, 2), &wsa);
		if ( res != NO_ERROR ) {
			printf("WSA init failed.\n");
			WSACleanup();
			return 1;
		}
	#endif
	signal(SIGINT, &signal_handler);
	int sck = socket(AF_INET, SOCK_STREAM, 0);
	if ( sck < 0 ) {
		printf("Socket init failed.\n");
		WSA_CLEAN();
		return 1;
	}
	struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	srv_addr.sin_port = htons(port);
	printf("Connecting to %s on port %d.\n", argv[1], port);
	int ret = connect(sck, (const struct sockaddr *)&srv_addr, sizeof(srv_addr));
	if ( ret < 0 ) {
		printf("Connection failed.\n");
		shutdown(sck, SHUT_RDWR);
		close(sck);
		WSA_CLEAN();
		return 1;
	}
	int term = 0;
	unsigned char buf[AD_BUF];
	// Main loop
	while ( 1 ) {
		// Receive command from server
		memset(buf, 0, sizeof(char) * AD_BUF);
		ret = recv(sck, (char *)buf, AD_BUF, 0);
		if ( ret <= 0 ) {
			printf("Receive error: connection lost.\n");
			break;
		}
		int len = 0;
		// parse server commands
		if ( buf[0] == AD_MULTI ) {
			term = parse_server(buf + 1, &len, 1);
			int i;
			for ( i = 0; i < len; i++ ) {
				buf[i] = buf[i + 1];
			}
		} else {
			term = parse_server(buf, &len, 0);
		}
		if ( term ) {
			break;
		}
		// Send response to server
		int ret = send(sck, (char *)buf, len, 0);
		if ( ret <= 0 ) {
			printf("Send error: connection lost.\n");
			break;
		}
	}
	shutdown(sck, SHUT_RDWR);
	close(sck);
	WSA_CLEAN();
	printf("Bye!\n");
	return 0;
}
