#include <semaphore.h>

#ifndef BARRIER_H
#define BARRIER_H

class Barrier {

	private:
		sem_t m;
		sem_t w;
		sem_t h;

		int counter;
		int capacity;

	public:
		Barrier(int size);
		int size();
		void wait();
};	
#endif
