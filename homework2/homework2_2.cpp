#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <iostream>

using namespace std;

struct showdown {
	int x;
	int y;
	int getMax() {
		return (x>=y) ? x : y;
	}
};

int max(int x, int y) {
	//return (x>=y) ? x : y;
	if(x>=y)
		return x;
	else
		return y;
}

void* getMax(void *arg1) {
	int** numbers = (int**)arg1;
	int temp = 0;
	temp =  (*numbers[0]>=*numbers[1]) ? *numbers[0] : *numbers[1];
	cout << temp << endl;
	pthread_exit(NULL);
}

int fightToTheDeath(int numCompetitors, showdown* showdowns, int** nums) {
	int i,j; int numShowdowns;
	int* numbers = *nums;
	
	while( numCompetitors > 1 ) {
		numShowdowns = numCompetitors / 2;
		
		for(i=0;i<numShowdowns;i++) {
			//*numbers[i] = showdowns[i/2]->getMax();
			cout << "-------- \n";
			cout << "COMPETITOR X = " << showdowns[i].x << "\n";
			cout << "COMPETITOR Y = " << showdowns[i].y << "\n";
			numbers[i] = max(showdowns[i].x, showdowns[i].y);
			cout << "numbers[" << i << "]  VALUE:" << numbers[i] << "\n";
			if (i>0)
				cout << "previous: " << numbers[i-1] << "\n";
		}	
		numCompetitors = numCompetitors/2;
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
		if(i%2 == 0) {
			showdowns[i/2].x = numbers[i];
			cout << "showdowns[" << i/2 << "] x = " << showdowns[i/2].x << "\n";
		}
		else{
			showdowns[i/2].y = numbers[i];
			cout << "showdowns[" << i/2 << "] y = " << showdowns[i/2].y << "\n";
		}
    i++;
  }

	// TEST
	int numCompetitors = i;
	cout << "----- READING BACK -----\n";
	for (i=0; i<numCompetitors/2; i++) {
		cout << "number: " << numbers[i] << "\n";
		cout << "showdowns[" << i << "] x = " << showdowns[i].x << "\n";
		cout << "showdowns[" << i << "] y = " << showdowns[i].y << "\n";
	}

	/*
	pthread_t tids[numCompetitors];

	// TEST : CREATE 1 THREAD TO COMPARE 1st 2 ELEMENTS
	pthread_create(&tids[0], NULL, getMax, (void*)&numbers[0]);
  pthread_join(tids[0], NULL);
	*/
	
	int numShowdowns = 0;
	
	int maximum = fightToTheDeath(numCompetitors, showdowns, &numbers);
	//cout << "MAX IS:" << *numbers[0] << "\n";
	return 0; 
}
