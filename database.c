#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdarg.h>
#include "database.h"
#include "util.h"

typedef enum { STATE_TYPE, STATE_VAR, STATE_VALUE_FIRST, STATE_VALUE } read_state;

// Log into txt file
int logprintf(log_status s, char * fmt, ...) {
	FILE *f = fopen("adventure.log", "a");
	if ( f == NULL ) {
		return -1;
	}
	int r;
	if ( s == LOG_END ) {
		r = fprintf(f, "\n\n");
	} else {
		char *stat;
		if ( s == LOG_OK ) {
			stat = "ok";
		} else if ( s == LOG_INFO ) {
			stat = "info";
		} else if ( s == LOG_WARN ) {
			stat = "warn";
		} else if ( s == LOG_ERR ) {
			stat = "error";
		} else if ( s == LOG_FATAL ) {
			stat = "FATAL";
		} else {
			stat = "undef";
		}
		char t[25];
		current_time(t);
		r = fprintf(f, "%s [%s] ", t, stat);
		va_list l;
		va_start(l, fmt);
		r += vfprintf(f, fmt, l);
		va_end(l);
	}
	if ( !fclose(f) ) {
		return -1;
	}
	return r;
}

// Load datas from database
int database_load(char *filename, void (* _construct)(object_prop_t *)) {
	FILE *f = fopen(filename, "r");
	if ( f == NULL ) {
		logprintf(LOG_FATAL, "unable to open file %s\n", filename);
		return 1;
	}
	char line[AD_BUF_SIZE];
	int line_n = 0;
	while ( fgets(line, AD_BUF_SIZE - 2, f) == line ) {
		// process line
		if ( line[0] != '#' ) {
			int l = strlen(line);
			line[l] = ',';
			line[l + 1] = '\0';
			read_state s = STATE_TYPE;
			var_type t = TYPE_INT;
			char current_string[AD_BUF_SIZE];
			int p = 0, current_int = 0, current_hex = 0, negate = 1, decimal = 1, str_q = 0;
			double current_double = 0;
			object_prop_t *object = (object_prop_t *)malloc(sizeof(object_prop_t)), *op, *prev;
			for ( int i = 0; line[i]; i++ ) {
				int error = 0;
				if ( s == STATE_TYPE ) {
					if ( is_char(line[i]) ) {
						// read type
						current_string[p++] = line[i];
					} else if ( line[i] == ':' ) {
						// type ok
						current_string[p] = '\0';
						p = 0;
						object->type = TYPE_OBJECT_T;
						object->name = (char *)malloc(sizeof(char) * ( strlen(current_string) + 1 ));
						strcpy(object->name, current_string);
						object->next = NULL;
						op = object;
						///printf("Object type: %s\n", current_string);
						s = STATE_VAR;
					} else if ( !is_whitespace(line[i]) && line[i] != '#' ) {
						error = 1;
					}
				} else if ( s == STATE_VAR ) {
					if ( is_char(line[i]) ) {
						// read var name
						current_string[p++] = line[i];
					} else if ( line[i] == '=' ) {
						current_string[p] = '\0';
						p = 0;
						prev = op;
						op->next = (object_prop_t *)malloc(sizeof(object_prop_t));
						op = op->next;
						op->name = (char *)malloc(sizeof(char) * ( strlen(current_string) + 1 ));
						strcpy(op->name, current_string);
						op->next = NULL;
						///printf("\tvariable %s: ", current_string);
						s = STATE_VALUE_FIRST;
					} else if ( !is_whitespace(line[i]) && line[i] != ',' && line[i] != '#' ) {
						error = 1;
					}
				} else if ( s == STATE_VALUE_FIRST ) {
					s = STATE_VALUE;
					str_q = 0;
					if ( line[i] == '"' ) {
						// gonna be string
						str_q = 1;
						t = TYPE_STR;
					} else if ( line[i] == '\\' ) {
						// gonna be string
						t = TYPE_STR_ESC;
					} else if ( line[i] == '.' ) {
						// gonna be double ( or string )
						decimal = 1;
						negate = 1;
						current_double = 0;
						t = TYPE_DOUBLE;
					} else if ( line[i] == 'x' || line[i] == 'X' ) {
						// gonna be hex ( or string )
						current_hex = 0;
						t = TYPE_HEX;
					} else if ( get_value(line[i]) >= 0x0a ) {
						// gonna be hex ( or string )
						current_hex = get_value(line[i]);
						t = TYPE_HEX;
					} else if ( line[i] == '-' ) {
						// gonna be negative int ( our double or string )
						negate = -1;
						current_int = 0;
						t = TYPE_INT;
					} else if ( get_value(line[i]) > 0 ) {
						// gonna be int ( our double or hex or string )
						negate = 1;
						current_int = get_value(line[i]);
						current_hex = current_int;
						t = TYPE_INT;
					} else if ( line[i] == ',' ) {
						///printf(" ...deleted: %s\n", op->name);
						free(op->name);
						free(op);
						prev->next = NULL;
						s = STATE_VAR;
					} else if ( !is_whitespace(line[i]) && line[i] != '0' && line[i] != ',' ) {
						// gonna be string
						t = TYPE_STR;
					} else {
						s = STATE_VALUE_FIRST;
					}
					if ( !is_whitespace(line[i]) && line[i] != '"' && line[i] != ',' && line[i] != '\\' ) {
						// store char anyway
						current_string[p++] = line[i];
					}
				} else if ( s == STATE_VALUE ) {
					if ( t != TYPE_STR_ESC ) {
						// read string
						if ( line[i] == '\\' ) {
							t = TYPE_STR_ESC;
						} else if ( ( line[i] == ',' || line[i] == '#' ) && !str_q && t == TYPE_STR ) {
							// read complete
							current_string[p] = '\0';
							p = 0;
							op->type = TYPE_STR;
							op->svalue = (char *)malloc(sizeof(char) * ( strlen(current_string) + 1 ));
							strcpy(op->svalue, current_string);
							///printf("(string) %s\n", current_string);
							s = STATE_VAR;
						} else if ( str_q && line[i] == '"' ) {
							str_q = 0;
						} else {
							current_string[p++] = line[i];
						}
					} else {
						// escaped whitespaces
						if ( line[i] == 't' ) {
							current_string[p++] = '\t';
						} else if ( line[i] == 'n' ) {
							current_string[p++] = '\n';
						} else {
							current_string[p++] = line[i];
						}
						if ( line[i] == '#' ) {
							line[i] = ' ';
						}
						t = TYPE_STR;
					}
					if ( t == TYPE_DOUBLE ) {
						// read double
						if ( get_value(line[i]) >= 0 && get_value(line[i]) <= 9 ) {
							current_double += get_value(line[i]) / pow(10.0, decimal++);
						} else if ( line[i] == ',' || line[i] == '#' ) {
							// read complete
							current_double *= negate;
							op->type = TYPE_DOUBLE;
							op->dvalue = current_double;
							///printf("(double) %.4f\n", current_double);
							p = 0;
							s = STATE_VAR;
						} else if ( !is_whitespace(line[i]) ) {
							t = TYPE_STR;
						}
					} else if ( t == TYPE_HEX ) {
						// read hex
						if ( get_value(line[i]) >= 0 ) {
							current_hex = 0x10 * current_hex + get_value(line[i]);
						} else if ( line[i] == ',' || line[i] == '#' ) {
							// read complete
							op->type = TYPE_HEX;
							op->hvalue = current_hex;
							///printf("(hex) %#x\n", current_hex);
							p = 0;
							s = STATE_VAR;
						} else if ( !is_whitespace(line[i]) ) {
							t = TYPE_STR;
						}
					} else if ( t == TYPE_INT ) {
						// read int
						if ( get_value(line[i]) >= 0x0a ) {
							if ( negate > 0 ) {
								// actually is hex
								current_hex = 0x10 * current_hex + get_value(line[i]);
								t = TYPE_HEX;
							} else {
								// no negative hex pls
								t = TYPE_STR;
							}
						} else if ( get_value(line[i]) >= 0 ) {
							current_hex = 0x10 * current_hex + get_value(line[i]);
							current_int = 10 * current_int + get_value(line[i]);
						} else if ( line[i] == '.' ) {
							// actually is double
							current_double = current_int;
							decimal = 1;
							t = TYPE_DOUBLE;
						} else if ( line[i] == ',' || line[i] == '#' ) {
							// read complete
							current_int *= negate;
							op->type = TYPE_INT;
							op->ivalue = current_int;
							///printf("(int) %d\n", current_int);
							p = 0;
							s = STATE_VAR;
						} else if ( !is_whitespace(line[i]) ) {
							t = TYPE_STR;
						}
					}
				}
				// report error
				if ( error ) {
					logprintf(LOG_ERR, "unexpected '%c' in %s line %d char %d\n", line[i], filename, line_n + 1, i);
					break;
				}
				// comment block
				if ( line[i] == '#' ) {
					break;
				}
			}
			// get rid of objects with no properties
			if ( object->next == NULL ) {
				logprintf(LOG_WARN, "useless object '%s' in %s line %d\n", object->name, filename, line_n + 1);
				free(object->name);
				free(object);
			} else {
				// contstruct complete object
				_construct(object);
			}
		}
		line_n++;
	}
	if ( fclose(f) ) {
		logprintf(LOG_FATAL, "unable to close file %s\n", filename);
		return 1;
	}
	return 0;
}
