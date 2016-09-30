#include "barrier.h"

Barrier::Barrier(int size) {
	sem_init(&m,0,1);
  sem_init(&w,0,0);
  sem_init(&h,0,0);
	
	capacity = size;
	counter = 0;
}

int Barrier::size() {
	return capacity;
}

void Barrier::wait() {
	sem_wait(&m);
	counter++;
	
	int i;
	if(counter == capacity) {
		for(i=0; i<capacity-1; i++) {
			sem_post(&w);
			sem_wait(&h);
		}
		counter = 0;
		sem_post(&m);
		return;
	}
	sem_post(&m);
	sem_wait(&w);
	sem_post(&h);
}
