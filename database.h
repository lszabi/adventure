#ifndef AD_DATABASE_H
#define AD_DATABASE_H

#include <stdint.h>
#include "util.h"

// Object property types
typedef enum { 
	TYPE_STR,
	TYPE_STR_ESC,
	TYPE_INT,
	TYPE_HEX,
	TYPE_DOUBLE,
	TYPE_OBJECT_T
} var_type;

// Log entry statuses
typedef enum { 
	LOG_OK,
	LOG_INFO,
	LOG_WARN,
	LOG_ERR,
	LOG_FATAL,
	LOG_END
} log_status;

// List of object properties
typedef struct object_prop {
	var_type type;
	char *name;
	union {
		double dvalue;
		int ivalue;
		char *svalue;
		uint32_t hvalue;
	};
	struct object_prop *next;
} object_prop_t;

int logprintf(log_status, char *, ...);
int database_load(char *, void (*)(object_prop_t *));

#endif
