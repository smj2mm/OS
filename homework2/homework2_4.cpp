#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <iostream>

using namespace std;

/* GLOBALS */ 
struct threadStuff {
	int index;
};

pthread_barrier_t barrier;
int numCompetitors;
int* numbers;
threadStuff* threadStructs;


void* getMax(void *arg1) {
	threadStuff* info = (threadStuff*) arg1;
	int index = info->index;
	int r;
	for(r=1; r<numCompetitors; r*=2) {
		//*(info->x) = numbers[index];
		//*(info_>y) = numbers[index+r];
		numbers[index] = (numbers[index]>=numbers[index+r]) ? numbers[index] : numbers[index+r];
		index *=2;
		//if(index >= numCompetitors)
		//pthread_exit(NULL);
		pthread_barrier_wait(&barrier);
	}
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

int fightToTheDeathThreads(int numCompetitors) {
	
	pthread_t tids[numCompetitors];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_barrierattr_t barrier_attr;
	pthread_barrierattr_init(&barrier_attr);

	int numThreads = numCompetitors / 2;
	
	int i;
	pthread_barrier_init(&barrier, &barrier_attr, numThreads);
	
	for(i=0; i<numThreads; i++) {
		threadStructs[i].index = i*2;
		cout << "i is: " << i << " threadStructs[i].index = " << i*2 << "\n";
		pthread_create(&tids[i], &attr, getMax, (void*)&threadStructs[i]);
	}

	return numbers[0];
}

int main() {
  numbers = new int[16384];
	threadStructs = new threadStuff[8192];

  int i=0; string s = "";
  
	while(getline(cin, s)) {
    //numbers[i] = new int;
		if(s.empty()) {
			break;
		}
    numbers[i] = atoi(s.c_str());
    i++;
  }
	
	int numCompetitors = i;
	// TEST : CREATE 1 THREAD TO COMPARE 1st 2 ELEMENTS
	
	int numShowdowns = 0;
	
	//int maximum = fightToTheDeath(numCompetitors, &numbers);
	int maximum = fightToTheDeathThreads(numCompetitors);
	//int maximum = fightToTheDeath(numCompetitors, &numbers);
	//cout << "MAX IS:" << *numbers[0] << "\n";
	cout << "MAXIMUM IS: "<< maximum << "\n";
	return 0; 
}
