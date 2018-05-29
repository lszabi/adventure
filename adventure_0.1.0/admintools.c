#include "common.h"
#include "database.h"

/*
 * Adventure admin's tool
 * created for Adventure
 * by L Szabi 2014
 * 
 * Export and import databases to/from *.lst text files
 * Note: text files MUST have a newline at the end!
 */

#define FIELD_CHAR 1
#define FIELD_NUM 2
#define FIELD_HEX 3
#define FIELD_SHORT 4
#define FIELD_RAW 5

#ifndef MAX_INT
	#define MAX_INT 0x7FFFFFFF
#endif

typedef struct {
	char name[64];
	uint8_t size;
	uint8_t type;
	int max; // value if num or length if char
	int min;
} field_t;

const int player_fields_n = 9;
const field_t player_fields[] = {
	{
		.name = "hash",
		.type = FIELD_RAW,
		.size = sizeof(char) * 16,
	},
	{
		.name = "name",
		.size = sizeof(char) * 16,
		.type = FIELD_CHAR,
		.max = 15,
		.min = 1,
	},
	{
		.name = "x",
		.size = sizeof(short),
		.type = FIELD_SHORT,
		.max = 500,
		.min = -500,
	},
	{
		.name = "y",
		.size = sizeof(short),
		.type = FIELD_SHORT,
		.max = 500,
		.min = -500,
	},
	{
		.name = "xp",
		.size = sizeof(int),
		.type = FIELD_NUM,
		.max = MAX_INT,
		.min = 0,
	},
	{
		.name = "level",
		.size = sizeof(int),
		.type = FIELD_NUM,
		.max = MAX_INT,
		.min = 0,
	},
	{
		.name = "hp",
		.size = sizeof(int),
		.type = FIELD_NUM,
		.max = MAX_INT,
		.min = 0,
	},
	{
		.name = "movement_speed",
		.size = sizeof(int),
		.type = FIELD_NUM,
		.max = MAX_INT,
		.min = 0,
	},
	{
		.name = "flags",
		.size = sizeof(uint32_t),
		.type = FIELD_HEX,
		.max = MAX_INT,
		.min = 0,
	}
};

int read_line(FILE *f, char **values, char *buf) {
	if ( fgets(buf, 255, f) != NULL ) {
		if ( buf[0] == '[' && buf[strlen(buf) - 2] == ']') {
			buf++;
			buf[strlen(buf) - 2] = '\0';
			return str_explode(buf, values, ',');
		}
	}
	return -1;
}

int count_lines(FILE *f) {
	int ret = 0, c = 0;
	while ( ( c = fgetc(f) ) != EOF ) {
		if ( c == '\n' ) {
			ret++;
		}
	}
	rewind(f);
	return ret;
}

void import_db(char *db_name, void *database, size_t db_size, const field_t *fields, const int fields_n, int *counter_var) {
	char file_name[64];
	sprintf(file_name, "%s.lst", db_name);
	FILE *f = fopen(file_name, "r");
	if ( f == NULL ) {
		printf("Failed to open %s\n", file_name);
	} else {
		int rows = count_lines(f);
		int counter = 0;
		free(database);
		database = malloc(db_size * rows);
		for ( int i = 0; i < rows; i++ ) {
			char **values = (char **)malloc(sizeof(char *) * fields_n);
			char buf[256];
			int ret = read_line(f, values, buf);
			if ( ret > 0 ) {
				int k = 0, error = 0;
				void *p = database + db_size * counter;
				memset(p, 0, db_size);
				for ( int j = 0; j < fields_n; j++ ) {
					error = 1;
					if ( fields[j].type == FIELD_CHAR ) {
						if ( !RANGE(strlen(values[k]), fields[j].min, fields[j].max) ) {
							printf("Value is out of range for '%s' field %d on line %d.\n", db_name, j, i);
							break;
						}
						strcpy((char *)p, values[k]);
					} else if ( fields[j].type == FIELD_NUM ) {
						int e = 0;
						int d = tosint(values[k], &e);
						if ( e || !RANGE(d, fields[j].min, fields[j].max) ) {
							printf("Value is out of range for '%s' field %d on line %d.\n", db_name, j, i);
							break;
						}
						*(int *)p = d;
					} else if ( fields[j].type == FIELD_SHORT ) {
						int e = 0;
						int d = tosint(values[k], &e);
						if ( e || !RANGE(d, fields[j].min, fields[j].max) ) {
							printf("Value is out of range for '%s' field %d on line %d.\n", db_name, j, i);
							break;
						}
						*(short *)p = d;
					} else if ( fields[j].type == FIELD_HEX ) {
						int d = tohex(values[k]);
						if ( !RANGE(d, fields[j].min, fields[j].max) ) {
							printf("Value is out of range for '%s' field %d on line %d.\n", db_name, j, i);
							break;
						}
						*(uint32_t *)p = d;
					} else if ( fields[j].type == FIELD_RAW ) {
						if ( strlen(values[k]) != fields[j].size * 2 ) {
							printf("RAW value has invalid length for '%s' field %d on line %d.\n", db_name, j, i);
						}
						for ( int r = 0; r < fields[j].size; r++ ) {
							char v[3];
							v[0] = values[k][2 * r];
							v[1] = values[k][2 * r + 1];
							v[2] = '\0';
							((char *)p)[r] = tohex(v);
						}
					} else {
						printf("Invalid '%s' field type %d of field %d.\n", db_name, player_fields[j].type, j);
						break;
					}
					error = 0;
					k++;
					p += fields[j].size;
				}
				if ( error ) {
					break;
				}
				counter++;
			}
			free(values);
		}
		fclose(f);
		*counter_var = counter;
		printf("%d %s were imported.\n", counter, db_name);
	}
}

void export_db(char *db_name, void *database, size_t db_size, const field_t *fields, const int fields_n, int counter) {
	char file_name[64];
	sprintf(file_name, "%s.lst", db_name);
	FILE *f = fopen(file_name, "w");
	if ( f == NULL ) {
		printf("Failed to open %s\n", file_name);
	} else {
		for ( int i = 0; i < fields_n; i++ ) {
			fprintf(f, "%s, ", fields[i].name);
		}
		fprintf(f, "\n");
		for ( int i = 0; i < counter; i++ ) {
			void *p = database + db_size * i;
			int error = 0;
			fprintf(f, "[");
			for ( int j = 0; j < fields_n; j++ ) {
				if ( fields[j].type == FIELD_CHAR ) {
					fprintf(f, "%s", (char *)p);
				} else if ( fields[j].type == FIELD_NUM ) {
					fprintf(f, "%d", *(int *)p);
				} else if ( fields[j].type == FIELD_HEX ) {
					fprintf(f, "%02x", *(int *)p);
				} else if ( fields[j].type == FIELD_SHORT ) {
					fprintf(f, "%d", *(short *)p);
				} else if ( fields[j].type == FIELD_RAW ) {
					uint8_t *r = (uint8_t *)p;
					for ( int k = 0; k < fields[j].size; k++ ) {
						fprintf(f, "%02x", r[k]);
					}
				} else {
					printf("Invalid '%s' field type %d of field %d.\n", db_name, fields[j].type, j);
					error = 1;
					break;
				}
				if ( j != fields_n - 1) {
					fprintf(f, ",");
				}
				p += fields[j].size;
			}
			if ( error ) {
				break;
			}
			fprintf(f, "]\n");
		}
		fclose(f);
		printf("%d %s were exported.\n", counter, db_name);
	}
}

int main(int argc, char *argv[]) {
	printf("Adventure admin's tool for Adventure %d.%d.%d\n\n", AD_VER_MAJ, AD_VER_MIN, AD_VER_REV);
	if ( argc < 3 ) {
		printf("Usage:\n\n");
		printf("\tadmintools OPERATION DATABASE[,DATABASE...]\n\n");
		printf("operation:\timport, export\n");
		printf("database:\tplayers\n");
		return 1;
	}
	if ( !char_eq(argv[1], "export") && !char_eq(argv[1], "import") ) {
		printf("Invalid operation.\n");
		return 1;
	}
	char **operations = (char **)malloc(sizeof(char *) * 10);
	int n = str_explode(argv[2], operations, ',');
	int op_unknown = 0;
	int op_players = 0;
	for ( int i = 0; i < n; i++ ) {
		if ( char_eq(operations[i], "players") ) {
			op_players = 1;
		} else {
			printf("Unknown database %s.\n", operations[i]);
			op_unknown++;
		}
	}
	printf("\n");
	if ( op_unknown == n ) {
		printf("No operation needed.\n");
		return 1;
	}
	if ( database_open() < 0 ) {
		printf("Failed to open database.\n");
		return 1;
	}
	if ( database_backup() < 0 ) {
		printf("Failed to backup database.\n");
		return 1;
	}
	if ( char_eq(argv[1], "import") ) {
		if ( op_players ) {
			import_db("players", player_database, sizeof(player_database_t), player_fields, player_fields_n, &player_database_n);
		}
	} else if ( char_eq(argv[1], "export") ) {
		if ( op_players ) {
			export_db("players", player_database, sizeof(player_database_t), player_fields, player_fields_n, player_database_n);
		}
	}
	if ( database_close() < 0 ) {
		printf("Failed to close database.\n");
		return 1;
	}
	printf("\nDone.\n");
	return 0;
}