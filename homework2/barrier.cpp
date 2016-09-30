#include "barrier.h"

Barrier::Barrier(int size) {
	// set the mutex semaphore such that it will not block initially
	sem_init(&m,0,1);
	// set the waiter to wait
  sem_init(&w,0,0);
	// set the handshake to wait
  sem_init(&h,0,0);
	
	capacity = size;
	counter = 0;
}


void Barrier::wait() {
	sem_wait(&m);
	counter++;
	
	int i;
	// if the correct number of threads have signaled wait
	if(counter == capacity) {
		for(i=0; i<capacity-1; i++) {
			// signal the waiter and wait on the handshake
			sem_post(&w);
			sem_wait(&h);
		}
		counter = 0;
		// signal the mutex so next round can commence
		sem_post(&m);
		return;
	}
	// signal the mutex - next thread can go
	sem_post(&m);
	// put myself on the waiting queue
	sem_wait(&w);
	// signal the handshake
	sem_post(&h);
}
