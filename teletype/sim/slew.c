#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>



int main(int argc, char *argv[]) {
	int now = atoi(argv[1]);
	int target = atoi(argv[2]);
	int step = atoi(argv[3]);

	int dy = target - now;
	long error = 0;
	uint32_t D = (dy<<16) / step;
	uint32_t a = now << 16;

	printf("stepping: %d",D);

	printf("\n%d",now);

	while(step--) {
		if(step==0)
			now = target;
		else {
			a += D;
			now = (a >> 16);
		}
		printf("\n%d",now);
	}

	printf("\n");

	return 0;
}