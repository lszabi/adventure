#include "common.h"
#include "database.h"
#include "game.h"

// Adventure game by L Szabi 2013

static FILE *players = NULL;
static int new_players = 0; 
static user_t *users;
int players_n = 0;
player_db_t *players_db;

static FILE *items = NULL;
int items_n = 0;
item_t *items_db;

// Get file size
static int file_size(FILE *f) {
	fseek(f, 0, SEEK_END);
	int ret = ftell(f);
	fseek(f, 0, SEEK_SET);
	return ret;
}

// Is name valid?
static int validate_name(char *name) {
	int i;
	for ( i = 0; name[i]; i++ ) {
		char c = name[i];
		if ( !( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <='9' ) || c == ' ' || c == '_' || c == '-' || c == '.' ) ) {
			return 1;
		}
	}
	return 0;
}

// Initialize db, create files
void init_db(int db) {
	FILE *f = NULL;
	switch ( db ) {
		case 1:
			f = fopen("db/players.bin", "w");
			if ( f == NULL ) {
				printf("Error creaing file: players.bin\n");
			}
		break;
		case 2:
			f = fopen("db/items.bin", "w");
			if ( f == NULL ) {
				printf("Error creaing file: items.bin\n");
			}
		break;
	}
	fclose(f);
}

// Start database, check files
int start_db(int admin) {
	mkdir("db");
	players = fopen("db/players.bin", "rb+");
	if ( players == NULL ) {
		return 1;
	}
	int size = file_size(players);
	players_n = size / sizeof(player_db_t);
	players_db = (player_db_t*)malloc(size + REG_LIMIT * sizeof(player_db_t));
	fread(players_db, sizeof(player_db_t), players_n, players);
	if ( !admin ) {
		users = (user_t*)malloc(( players_n + REG_LIMIT ) * sizeof(user_t));
		int i;
		for ( i = 0; i < players_n + REG_LIMIT; i++ ) {
			users[i].db = &(players_db[i]);
			users[i].todo = init_list();
			users[i].time_rem = 0;
		}
	}
	items = fopen("db/items.bin", "rb+");
	if ( items == NULL ) {
		return 2;
	}
	size = file_size(items);
	items_n = size / sizeof(item_t);
	items_db = (item_t*)malloc(size);
	fread(items_db, sizeof(item_t), items_n, items);
	return 0;
}

// Close db
void close_db(int admin) {
	fseek(players, 0, SEEK_SET);
	fseek(items, 0, SEEK_SET);
	if ( !admin ) {
		int i;
		for ( i = 0; i < players_n; i++ ) {
			close_list(users[i].todo);
		}
		fwrite(players_db, sizeof(player_db_t), players_n, players);
	} else {
		fclose(players);
		players = fopen("db/players.bin", "wb");
		int i;
		for ( i = 0; i < players_n; i++ ) {
			if ( players_db[i].name[0] != '\0' ) {
				fwrite(&(players_db[i]), sizeof(player_db_t), 1, players);
			}
		}
		fclose(items);
		items = fopen("db/items.bin", "wb");
		for ( i = 0; i < items_n; i++ ) {
			if ( items_db[i].name[0] != (char)1 ) {
				fwrite(&(items_db[i]), sizeof(item_t), 1, items);
			}
		}
	}
	fclose(players);
	fclose(items);
}

// Register new player
int register_player(char *name) {
	if ( new_players == REG_LIMIT ) {
		return 1;
	}
	if ( strlen(name) > 15 || strlen(name) < 4 || !stricmp(name, "admin") || !stricmp(name, "root") ) {
		return 2;
	}
	if ( validate_name(name) ) {
		return 3;
	}
	int i;
	for ( i = 0; i < players_n; i++ ) {
		if ( !stricmp(players_db[i].name, name) ) {
			return 4;
		}
	}
	new_players++;
	memset(&(players_db[players_n]), 0, sizeof(player_db_t));
	strcpy(players_db[players_n].name, name);
	users[players_n].todo = init_list();
	users[players_n].time_rem = 0;
	srand((int)time(NULL));
	players_db[players_n].x = (short)random(0, 1000) - 500;
	players_db[players_n].y = (short)random(0, 1000) - 500;
	players_n++;
	return 0;
}

// Pointer to user
user_t *get_user(int idx) {
	return &(users[idx]);
}

// Return player_db pointer
user_t *get_player_db(char *name) {
	int i;
	for ( i = 0; i < players_n; i++ ) {
		if ( !stricmp(players_db[i].name, name) ) {
			return &(users[i]);
		}
	}
	return NULL;
}

// List ( fifo )

// Init list
list_t *init_list() {
	list_t *lst = (list_t*)malloc(sizeof(list_t));
	lst->data = 0;
	lst->prev = 0;
	lst->len = 0;
	lst->next = 0;
	return lst;
}

// Push to the end of the list
void push_list(unsigned char *c, int len, list_t *lst) {
	while ( lst->next ) {
		lst = lst->next;
	}
	if ( lst->len ) {
		list_t *lst_new = init_list();
		lst_new->prev = lst;
		lst->next = lst_new;
		lst = lst_new;
	}
	lst->next = 0;
	lst->data = (unsigned char*)malloc(sizeof(unsigned char) * len);
	memcpy(lst->data, c, len);
	lst->len = len;
}

// Pop from the beginning of te list
list_t *pop_list(unsigned char *c, int *len, list_t *lst) {
	*len = lst->len;
	memcpy(c, lst->data, lst->len);
	free(lst->data);
	lst->data = 0;
	lst->len = 0;
	if ( lst->next ) {
		lst = lst->next;
		free(lst->prev);
		lst->prev = 0;
	}
	return lst;
}

// Close list
void close_list(list_t *lst) {
	list_t *tmp = lst;
	while ( lst->next ) {
		tmp = lst->next;
		free(lst);
		lst = tmp;
	}
	free(tmp);
}