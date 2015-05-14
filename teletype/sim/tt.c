#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "teletype.h"



int main(int argc, char *argv[]) {
	char *in;
	time_t t;
	error_t status;
	int i;

	char *s;

	srand((unsigned) time(&t));

	tele_command_t stored;
	stored.data[0].t = OP;
	stored.data[0].v = 2;
	stored.data[1].t = NUMBER;
	stored.data[1].v = 8;
	stored.data[2].t = NUMBER;
	stored.data[2].v = 10;
	stored.separator = -1;
	stored.l = 3;

	in = malloc(256);

	printf("teletype. (blank line quits)");

	do {
		printf("\n\n> ");
		fgets(in, 256, stdin);

		i = 0;
		while(in[i]) {
		    in[i] = toupper(in[i]);
		    i++;
		 }

		status = parse(in);
		if(status == E_OK) {
			status = validate(&temp);
			printf("\nvalidate: %s", tele_error(status));
			if(status == E_OK)
				process(&temp);
		}
		else {
			printf("\nERROR: %s", tele_error(status));
		}

		tele_tick();
	}
	while(in[0] != 10);

	free(in);
	
	printf("\nstored process: ");
	process(&stored);

	printf("\n(teletype exit.)\n");
}