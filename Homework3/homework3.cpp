/*
* Steven Jenny
* 11/8/2016
* CS 4414
* DOS File System
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>


void createTokenArray(char* token, char** tokens, int* numTokens) {
	int i = 0;
	while(token != NULL) {
		// create space, add token
		tokens[i] = malloc(sizeof(token));
		tokens[i] = new token;
		token = strtok(NULL, " ");
		i++;
		// update number of tokens
		*numTokens=i;
	}
}


int main() {
	
  while(1) {
    // make room for 100 characters for each token user enters
    char* input = (char*)(malloc(102 * sizeof(char)));
		memset(input, 0, 102 * sizeof(char));
    // printf("$ "); //INCLUDED FOR TESTING
		// store into input
    char* stillText = fgets(input, 102, stdin);
    // Check for EOF token
		if(!stillText)
			exit(0);
		
		if(!strchr(input, '\n'))     // newline does not exist
    	while(fgetc(stdin)!='\n'); // discard until newline
		
		input[strcspn(input, "\n")] = 0;
    
		// For ensuring extra characters properly discarded
		int len = (int) strlen(input);
		if(len > 100)
			memset(input, 0, 102 * sizeof(char));
		
		// check for exit string
		if(strcmp("exit", input)==0)
			exit(0);

		char** tokens;
		tokens = malloc(101 * (sizeof(char*)));
		memset(tokens,0, 101 * sizeof(char*));

		char* token = strtok(input, " ");
		int numTokens = 0;
		createTokenArray(token, tokens, &numTokens);
		
		free(input);
		free(tokens);
  }
  return 0;
}
