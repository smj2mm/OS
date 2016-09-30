/*
* Steven Jenny
* 9/29/2016
* CS 4414
* Machine Problem 2 - Finding the Maximum Value
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <iostream>
#include "barrier.h"
#include <time.h>
#include <algorithm>

using namespace std;

/* GLOBALS */ 
struct threadStuff {
	int index;
	Barrier* barrier;
};

int numCompetitors;
int* numbers;
threadStuff* threadStructs;
//Barrier globalBarrier;


int fightToTheDeath() {
	/* Implementation of basic algorithm without threads */
	int i;
	int r;
	
	// Process the correct number of rounds 
	for(r=1; r<numCompetitors; r*=2) {
		// Compare inputs that are "r" apart
		for(i=0; i<numCompetitors-r; i++) {
			numbers[i] = max(numbers[i], numbers[i+r]);
		}
	}	
	return numbers[0];
}

void* getMax(void *arg1) {
	/* Thread function for getting max */
	threadStuff* info = (threadStuff*) arg1;
	int index = info->index;
	int r;
	// Each round, compare elements that are 2^(round number) apart
	for(r=1; r<numCompetitors; r*=2) {
		// Check if the thread still needs to do computation
		if(index<numCompetitors) {
			// Manipulate the numbers array by placing the greater number in the lower of the compared indices
			numbers[index] = (numbers[index] >= numbers[index+r]) ? numbers[index] : numbers[index+r];
			// On the next iteration, index should have doubled to ensure "tournament style"
			index *=2;
		}
		// Wait for round to complete before proceeding
		info->barrier->wait();
	}
}

int main() {
	// Make room for 16384 numbers and half as many threads - 16384 is next power of two above 10K
  numbers = new int[16384];
	threadStructs = new threadStuff[8192];

  int i=0; string s = "";
  
	// Process input from file or from command line
	while(getline(cin, s)) {
		if(s.empty()) {
			break;
		}
    numbers[i] = atoi(s.c_str());
    i++;
  }
	
	// since i increments at end of parsing, i is the number of elements in the numbers array
	numCompetitors = i;

	// create thread id's for all competitors
	pthread_t tids[numCompetitors];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	// set threads to be detached so they won't anticipate a join
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	// need half as many threads as numbers
	int numThreads = numCompetitors / 2;

	// create a barrier of size one greater than number of threads so main program can wait for final result
	Barrier myBarrier(numThreads+1);

	// for timing algorithm
	// clock_t begin = clock();

	int r;
	// create threads - pass in the lower index in array for comparison and reference to barrier in struct
	for(i=0; i<numThreads; i++) {
		threadStructs[i].index = i*2;
		threadStructs[i].barrier = &myBarrier;
		pthread_create(&tids[i], &attr, getMax, (void*)&threadStructs[i]);
	}	

	// wait in main program thread
	for(r=1; r<numCompetitors; r*=2) {
		myBarrier.wait();
	}
	
	cout << numbers[0] << "\n";
	// for timing comparison
	/*
	clock_t end = clock();
	double time_spent = (double)(end-begin) / CLOCKS_PER_SEC;
	
	cout << "THREAD METHOD: " << time_spent << "\n";

	begin = clock();
	cout << "MAXIMUM TRADITIONAL IS: " << fightToTheDeath() << "\n";
	end = clock();
	time_spent = (double)(end-begin) / CLOCKS_PER_SEC;
	
	cout << "TRADITIONAL METHOD: " << time_spent << "\n";
	
	begin = clock();
	cout << "MAXIMUM PREMADE IS: " << *std::max_element(numbers, numbers + numCompetitors -1) << "\n";
	end = clock();
	time_spent = (double) (end-begin) / CLOCKS_PER_SEC;
	cout << "ONE MORE: " << time_spent << "\n";
	*/
	return 0; 
}
