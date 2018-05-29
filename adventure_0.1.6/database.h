#ifndef DATABASE_H
#define DATABASE_H

// Adventure game by L Szabi 2013

#define REG_LIMIT 16

typedef struct list {
	unsigned char *data;
	int len;
	struct list *prev;
	struct list *next;
} list_t;

typedef struct {
	unsigned int id, stack;
} __attribute__((packed)) inv_t;

typedef struct {
	char name[16];
	unsigned char passwd[16];
	short x, y;
	unsigned int flags;
	unsigned char task;
	time_t task_started;
	int task_arg;
	char reserved[11];
	unsigned int max_hp, atk, def, spd, dmg, hp, mv_spd, xp, lvl;
	inv_t invertory[64];
} __attribute__((packed)) player_db_t;

typedef struct {
	player_db_t *db;
	list_t *todo;
	int time_rem;
} user_t;

typedef struct {
	unsigned int id;
	unsigned int value;
	char name[24];
} __attribute__((packed)) item_t;

extern int players_n, items_n;

void init_db(int);
int start_db(int);
void close_db(int);

int register_player(char *);
user_t *get_user(int);
user_t *get_player_db(char *);

// List

list_t *init_list();
void push_list(unsigned char *, int, list_t *);
list_t *pop_list(unsigned char *, int *, list_t *);
void close_list(list_t *);

#endif // DATABASE_H