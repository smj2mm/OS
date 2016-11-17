#include <stdio.h>
#include <unistd.h>

int x = 10;

int main(void) {
	pid_t pid=-1;
	printf(" (0) x = %i\n", x);
	pid = fork();
	printf(" (1) x = %i\n", x);
	if(pid==0) {
		printf(" (2) x = %i\n", x);
		x = x + 5;
		printf(" (3) x = %i\n", x);
	}
	else {
		int i;
		while(i<100000000)
			i++;
	}	
	printf(" (5) x = %i\n", x);
	printf(" (4) x = %i\n", x);
}
