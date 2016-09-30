#include <semaphore.h>

#ifndef BARRIER_H
#define BARRIER_H

class Barrier {

	private:
		// mutex
		sem_t m;
		// waiter
		sem_t w;
		// handshake
		sem_t h;

		int counter;
		int capacity;

	public:
		Barrier(int size);
		void wait();
};	
#endif
