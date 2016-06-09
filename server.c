#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "database.h"
#include "player.h"

// Construct an object from the object properties
void construct_object(object_prop_t *p) {
	if ( !strcmp(p->name, "player") ) {
		build_player(p->next);
	} else {
		// Delete from memory
		while ( p ) {
			free(p->name);
			if ( p->type == TYPE_STR ) {
				free(p->svalue);
			}
			object_prop_t *f = p->next;
			free(p);
			p = f;
		}
		return;
	}
	free(p->name);
	free(p);
}

int main(void) {
	printf("Reading from db:\n\n");
	database_load("test.db", construct_object);
	printf("\n");
	logprintf(LOG_END, "");
	return 0;
}