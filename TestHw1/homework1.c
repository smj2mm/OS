/*
* Steven Jenny
* 9/28/2016
* CS 4414
* Writing Your Own Shell
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>

#define STD_IN  0
#define STD_OUT 1

struct TokenGroupInfo {
/*All information needed for reference within a tokengroup*/	
	char* command;			// First token following pipe
	char** args;				// Command and following arguments
	char* fileSpecOut;	// Token following '>'
	char* fileSpecIn;		// Token following '<' 
	int fds[2];					// File descriptors for pipes
};

void createTokenArray(char* token, char** tokens, int* numTokens) {
	int i = 0;
	while(token != NULL) {
		// create space, add token
		tokens[i] = malloc(sizeof(token));
		tokens[i] = token;
		token = strtok(NULL, " ");
		i++;
		// update number of tokens
		*numTokens=i;
	}
}

int isOperator(char* token) {
	/* Returns 1 if is one of '<', '>', and '|' */
	if(strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, "|") == 0)
		return 1;
	else	
		return 0;
}

int isValidWord(char* token) {
	/* A-Z, a-z, 0-9, dash, dot, forward slash, and underscore are only valid characters (after spaces removed)*/
	regex_t preg;
	int reti = 0;
	// Create and compile regular expression to satisfy above conditions
	char* regString = "^[0-9a-zA-Z_/.-]*$";
	reti = regcomp(&preg, regString, 0);
	reti = regexec(&preg, token, 0, NULL, 0);
	if(reti) {
		return 0;
	}
	return 1;
}

int isValidCombination(char** tokens, int numTokens) {
	/* Check for valid individual tokens and valid combinations*/
	int i;
	for(i=0; i<numTokens; i++) {
		/* Check if all individual tokens valid */
		if(!(isValidWord(tokens[i]) || isOperator(tokens[i]))) {
			return 0;
		}
		/* No tokens at beginning or end */
		if(i==numTokens || i==0) {
			if(isOperator(tokens[i])) {
				return 0;
			}
		}
		/* No operators right next to each other*/
		if(i<numTokens-1) {
			if(isOperator(tokens[i]) && isOperator(tokens[i+1]))
				return 0;
		}
	}
	return 1;
}

void destroyTokenGroups(struct TokenGroupInfo* tokenGroups, int lastGroupIndex) {
	/* Free malloc'd space for token groups */
	int i;
	for(i=0; i<=lastGroupIndex; i++) {
		free(tokenGroups[i].args);
	}
	free(tokenGroups);
}

void createTokenGroups(struct TokenGroupInfo* tokenGroups, int* lastGroupIndex, char** tokens, int numTokens) {
	/* Creates pointers to token groups, which store information on tokens in between or outside of pipes*/
	// Specify address of command and allocate space for arguments
	tokenGroups[0].command = tokens[0];
	tokenGroups[0].args = (char**)malloc(50*sizeof(char*));
	memset(tokenGroups[0].args,0,50*sizeof(char*));
	
	// i iterates through tokens, 
	// j is index for tokenGroup number
	// k is index for argument number
	int i; int j=0; int k=0;
	int processingArguments = 1;	// Used as flag for whether still processing arguments

	for(i = 0; i<numTokens; i++) {
		// if we see a pipe, start setting up the next token group	
		if(strcmp(tokens[i], "|") == 0) {	
			processingArguments = 1;
			tokenGroups[j+1].command = tokens[i+1];
			j++;
			// create argument array for next token group
			tokenGroups[j].args = (char**)malloc(50*sizeof(char*));
			memset(tokenGroups[j].args,0,50*sizeof(char*));
			k=0;
		}

		else {
			// if the argument array is still being created
			if(processingArguments) {
				if(tokens[i+1]) {
					// if the next token is an operator, there are no more arguments
					if(isOperator(tokens[i+1])) {
					  // if the next thing is an operator
						processingArguments = 0;
					}
				}
				// append arguments to argument array
				tokenGroups[j].args[k] = tokens[i];
				k++;
			}
			// check for redirect in, and if present, set pointer to file
			if(strcmp(tokens[i], "<") == 0) {
				tokenGroups[j].fileSpecIn = tokens[i+1];
			}
			// check for redirect out, and if present, set pointer to file
			else if(strcmp(tokens[i], ">") == 0)
				tokenGroups[j].fileSpecOut = tokens[i+1];
		}
	}
	// set the value of the last group index for later indexing
	*lastGroupIndex = j;
}

void printTokenGroups(struct TokenGroupInfo* tokenGroups, int lastGroupIndex) {
  /*
	* Print out all info for each token group:
		char* command;
		char** args;
		char* fileSpecOut;
		char* fileSpecIn;
		int fds[2];
	*/
	int i, j;
  for(i=0; i<=lastGroupIndex; i++) {
    j=0; char * iterator = ""; 
    printf("--- TOKENGROUP %d ---\n", i);
		printf("   COMMAND: %s\n", tokenGroups[i].command);
		
		j=0;
		while(tokenGroups[i].args[j]) {	
			printf("   ARG %d: %s\n", j, tokenGroups[i].args[j]);
			j++;
		}
		// Redirect in
		if(tokenGroups[i].fileSpecIn) {
			printf("   REDIR IN: %s\n", tokenGroups[i].fileSpecIn);
		}
		else {
			printf("   NO REDIR IN --\n");
		}
 		// Redirect out
		if(tokenGroups[i].fileSpecOut) {
			printf("   REDIR OUT: %s\n", tokenGroups[i].fileSpecOut);
		}
		else {
			printf("   NO REDIR OUT --\n");
		}
  }
}


void checkGroups(struct TokenGroupInfo* tokenGroups, int lastGroupIndex) {
	int i;
	for(i=0; i<lastGroupIndex; i++) {
		// Make number of groups - 1 pipes
		if(i==0 || i==lastGroupIndex) {
			if(i==0) {
				// First group - could have redirect in 
				if(tokenGroups[i].fileSpecOut) {
					printf("%s\n", "invalid input");
				}
			}
			if(i==lastGroupIndex) {
				// Last group - could have redirect out
				if(tokenGroups[i].fileSpecIn) {
					printf("%s\n", "invalid input");
				}
			}
		}
		else { 
			// Neither first nor last group - no redirects
			if(tokenGroups[i].fileSpecIn || tokenGroups[i].fileSpecOut) {	
				printf("In %s\n Out %s\n",tokenGroups[i].fileSpecIn, tokenGroups[i].fileSpecOut);
				printf("%s\n", "invalid input");
			}
		}
	}
}

void createPipes(struct TokenGroupInfo* tokenGroups, int lastGroupIndex) {
	/* create all pipes - set file descriptors of tokenGroups */
	int i;
	int buffer[2];

	// SPECIAL CASE - FIRST GROUP READ
	if (tokenGroups[0].fileSpecIn)
		tokenGroups[0].fds[0] = open(tokenGroups[0].fileSpecIn, O_RDONLY);
	else
		tokenGroups[0].fds[0] = -1;
	
	for (i=0; i<lastGroupIndex; i++) {
		pipe(buffer);
		tokenGroups[i].fds[1] = buffer[1]; 			// write pipe
		tokenGroups[i+1].fds[0] = buffer[0];		// read	pipe
	}

	// SPECIAL CASE - LAST GROUP WRITE
	if (tokenGroups[lastGroupIndex].fileSpecOut)
		tokenGroups[lastGroupIndex].fds[1] = open(tokenGroups[lastGroupIndex].fileSpecOut, O_WRONLY | O_CREAT | O_TRUNC, 00664);
	else
		tokenGroups[lastGroupIndex].fds[1] = -1;
}


void printPipes(struct TokenGroupInfo* tokenGroups, int lastGroupIndex) {
	/* print out information on all pipes - included for testing */
	int i;
	printf("----CREATED PIPES----\n");
	printf("HIGHEST TOKEN GROUP INDEX: %d\n", lastGroupIndex);

	// SPECIAL CASE - FIRST GROUP READ
	printf("  tokenGroups[0].fds[0] = %d\n", tokenGroups[0].fds[0]);	
	
	// AVERAGE CASE
	for (i=0; i<lastGroupIndex; i++) {
		printf("    tokenGroups[%d].fds[1] = %d\n", i, tokenGroups[i].fds[1]);
		printf("  tokenGroups[%d].fds[0] = %d\n", i+1, tokenGroups[i+1].fds[0]);
	}

	// SPECIAL CASE - LAST GROUP WRITE
	printf("    tokenGroups[%d].fds[1] = %d\n", lastGroupIndex, tokenGroups[lastGroupIndex].fds[1]);	
}

void handleTokenGroups(struct TokenGroupInfo* tokenGroups, int lastGroupIndex) {	
	/* main function for forking, duping, and execing */
	int i; int j;

	for (i=0; i<=lastGroupIndex; i++) {
		int p = fork();
		if (p==0) {
			// THIS IS THE CHILD
			// dup2 only if the pipe is open
			if (tokenGroups[i].fds[0] > -1) 
				dup2(tokenGroups[i].fds[0], STDIN_FILENO);
			if (tokenGroups[i].fds[1] > -1) 
				dup2(tokenGroups[i].fds[1], STDOUT_FILENO);
			
			for (j=0; j<lastGroupIndex; j++) {
				// close all other pipes not used by child
				if (j!=i) {
					//only attempt to close if the file descriptor is valid
					if (tokenGroups[j].fds[0] > -1)
						close(tokenGroups[j].fds[0]);	// close read to pipe j
					if (tokenGroups[j].fds[1] > -1)
						close(tokenGroups[j].fds[1]); // close write for pipe j
				}
			}
			execve(tokenGroups[i].command, tokenGroups[i].args, NULL);
		}
		else {
			// THIS IS THE PARENT
			// for each valid file dsecriptor, close the pipes
			if(tokenGroups[i].fds[0] > -1)
				close(tokenGroups[i].fds[0]);
			if(tokenGroups[i].fds[1] > -1)
				close(tokenGroups[i].fds[1]);
			int status;
			// suspend execution until child has changed state
			waitpid(p, &status, 0);
			fprintf(stderr, "%d\n", status);
		}
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

		//int* pids = (int*)(malloc(50*sizeof(int)));
		char** tokens;
		tokens = malloc(101 * (sizeof(char*)));
		memset(tokens,0, 101 * sizeof(char*));

		char* token = strtok(input, " ");
		int numTokens = 0;
		createTokenArray(token, tokens, &numTokens);
		
		if(!isValidCombination(tokens, numTokens) || len > 100) {
			printf("invalid input\n");
		}
		else if(strcmp("", input)==0) {
			/*do nothing with empty input*/; 
		}
		else {
			struct TokenGroupInfo* tokenGroups;
			tokenGroups = (struct TokenGroupInfo*)(malloc(20 * (sizeof(struct TokenGroupInfo))));
			int lastGroupIndex = 0;
			//printf("%s\n", "CREATING TOKENGROUPS");
			createTokenGroups(tokenGroups, &lastGroupIndex, tokens, numTokens);
			//printf("%s\n", "END CREATING TOKENGROUPS");
			checkGroups(tokenGroups, lastGroupIndex);
			//printTokenGroups(tokenGroups, lastGroupIndex);		
			createPipes(tokenGroups, lastGroupIndex);
			//printPipes(tokenGroups, lastGroupIndex);
			handleTokenGroups(tokenGroups, lastGroupIndex);
			destroyTokenGroups(tokenGroups, lastGroupIndex); // garbage collection
		}	
		free(input);
		//free(pids);
		free(tokens);
  }
  return 0;
}
