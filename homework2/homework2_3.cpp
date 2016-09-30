#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <iostream>

using namespace std;

pthread_barrier_t barrier;

struct showdown {
	int* x;
	int* y;
};

int max(int x, int y) {
	return (x>=y) ? x : y;
}

void* getMax(void *arg1) {
	showdown* thisShowdown = (showdown*) arg1;
	*(thisShowdown->x) = (*(thisShowdown->x)>=*(thisShowdown->y)) ? *(thisShowdown->x) : *(thisShowdown->y);
	pthread_barrier_wait(&barrier);
}

int fightToTheDeath(int numCompetitors, int** nums) {
	int i;
	int r;
	int* numbers = *nums;
	
	for(r=1; r<numCompetitors; r*=2) {
		//cout << "r value: " << r << "\n";
		for(i=0; i<numCompetitors-r; i++) {
			numbers[i] = max(numbers[i], numbers[i+r]);
			cout << "WINNER: numbers[" << i << "] = " << numbers[i] << "\n";
		}
	}	
	return numbers[0];
}

int fightToTheDeathThreads(int numCompetitors, int** nums, showdown* showdowns) {
	int i;
	int r;
	int* numbers = *nums;
	
	pthread_t tids[numCompetitors];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_barrierattr_t barrier_attr;
	pthread_barrierattr_init(&barrier_attr);

	int numThreads = numCompetitors / 2;
	for(r=1; r<numCompetitors; r*=2) {
		//cout << "PRINT NUMTHREADS: " << numThreads << "\n";
		pthread_barrier_init(&barrier, &barrier_attr, numThreads + 1);
		//cout << "r value: " << r << "\n";
		for(i=0; i<numCompetitors-r; i++) {
			showdowns[i].x = &numbers[i];
			showdowns[i].y = &numbers[i+r];
			pthread_create(&tids[i], &attr, getMax, (void*)&showdowns[i]);
			//cout << "WINNER: numbers[" << i << "] = " << numbers[i] << "\n";
		}
		pthread_barrier_wait(&barrier);
		numThreads /= 2;
	}

	for(i=0; i<numThreads/2; i++) {
  	pthread_join(tids[i], NULL);
	}	
	return numbers[0];
}

int main() {
  int* numbers = new int[16384];
	showdown* showdowns = new showdown[8192];
	
  int i=0; string s = "";
  
	while(getline(cin, s)) {
    //numbers[i] = new int;
    numbers[i] = atoi(s.c_str());
    i++;
  }
	
	int numCompetitors = i;
	// TEST : CREATE 1 THREAD TO COMPARE 1st 2 ELEMENTS
	
	int numShowdowns = 0;
	
	//int maximum = fightToTheDeath(numCompetitors, &numbers);
	//int maximum = fightToTheDeathThreads(numCompetitors, &numbers, showdowns);
	int maximum = fightToTheDeath(numCompetitors, &numbers);
	//cout << "MAX IS:" << *numbers[0] << "\n";
	cout << "MAXIMUM IS: "<< maximum << "\n";
	return 0; 
}
