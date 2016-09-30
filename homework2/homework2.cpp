#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <iostream>
#include "barrier.h"

using namespace std;

/* GLOBALS */ 
struct threadStuff {
	int index;
	Barrier* barrier;
};

pthread_barrier_t barrier;
int numCompetitors;
int* numbers;
threadStuff* threadStructs;
sem_t returnWait;
//Barrier globalBarrier;


int fightToTheDeath() {
	int i;
	int r;
	
	for(r=1; r<numCompetitors; r*=2) {
		//cout << "r value: " << r << "\n";
		for(i=0; i<numCompetitors-r; i++) {
			numbers[i] = max(numbers[i], numbers[i+r]);
			//cout << "WINNER: numbers[" << i << "] = " << numbers[i] << "\n";
		}
	}	
	return numbers[0];
}

void* getMax(void *arg1) {
	threadStuff* info = (threadStuff*) arg1;
	int index = info->index;
	int r;
	for(r=1; r<numCompetitors; r*=2) {
		
		if(index<numCompetitors) {
			numbers[index] = (numbers[index] >= numbers[index+r]) ? numbers[index] : numbers[index+r];
			//cout << numbers[index] << "\n";
			index *=2;
		}
		//pthread_barrier_wait(&barrier);
		info->barrier->wait();
		//globalBarrier.wait();
	}
	/*
	if(index==0) {
		sem_post(&returnWait);		
	}*/
}

int main() {

	sem_init(&returnWait,0,1);
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
	
	numCompetitors = i;
	// TEST : CREATE 1 THREAD TO COMPARE 1st 2 ELEMENTS
	
	int numShowdowns = 0;
	
	//int maximum = fightToTheDeath(numCompetitors, &numbers);
	//int maximum = fightToTheDeathThreads(numCompetitors);
	//int maximum = fightToTheDeath(numCompetitors, &numbers);
	//cout << "MAX IS:" << *numbers[0] << "\n";

	pthread_t tids[numCompetitors];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	//pthread_barrierattr_t barrier_attr;
	//pthread_barrierattr_init(&barrier_attr);

	int numThreads = numCompetitors / 2;
	cout << "numThreads + 1 = " << numThreads+1 << "\n";
	cout << "numCompetitors = " << numCompetitors << "\n";

	//pthread_barrier_init(&barrier, &barrier_attr, numThreads+1);
	Barrier myBarrier(numThreads+1);
	//globalBarrier(numThreads+1);

	int r;
	for(i=0; i<numThreads; i++) {
		threadStructs[i].index = i*2;
		threadStructs[i].barrier = &myBarrier;
		cout << "barrier size: " << threadStructs[i].barrier->size() << "\n";
		cout << "i is: " << i << " threadStructs[i].index = " << i*2 << "\n";
		pthread_create(&tids[i], &attr, getMax, (void*)&threadStructs[i]);
	}	
	
	for(r=1; r<numCompetitors; r*=2) {
		cout << "r = " << r << "\n";
		myBarrier.wait();
	}
	
	cout << "MAXIMUM IS: " << numbers[0] << "\n";
	cout << "MAXIMUM TRADITIONAL IS: " << fightToTheDeath() << "\n";
	return 0; 
}
