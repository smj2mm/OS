#include <stdio.h>
#include <stdbool.h>

bool test_and_set(bool *target) {
	bool rv = *target;
	*target = true;
	return rv;
}

void synchronization(bool *lock) {
	do{
		while(test_and_set(lock)){
			printf("-");
		}
		*lock = false;
	}while(true);
}

int main() {
	bool lock = false;
	test_and_set(&lock);
	if(lock==true) {
		printf("bla\n");
	}
	lock = false;
	synchronization(&lock);
	return 0;
}
