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
	char* command;
	char** args;
	char* fileSpecOut;
	char* fileSpecIn;
	int fds[2];
};

void createTokenArray(char* token, char** tokens, int* numTokens) {
	int i = 0;
	while(token != NULL) {
		tokens[i] = malloc(sizeof(token));
		tokens[i] = token;
		//printf("%s \n", token);
		token = strtok(NULL, " ");
		i++;
		*numTokens=i;
    //printf("%s \n", input);
	}
}

int isOperator(char* token) {
	if(strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, "|") == 0)
		return 1;
	else	
		return 0;
}

int isValidWord(char* token) {
	// if(strstr(sent, word) != NULL) - strstr returns pointer to the start of the word in sent if the word word is found
	// A-Z, a-z, 0-9, dash, dot, forward slash, and underscore are only valid characters (after spaces removed)
	regex_t preg;
	int reti = 0;
	char* regString = "^[0-9a-zA-Z_/.-]*$";
	reti = regcomp(&preg, regString, 0);
	reti = regexec(&preg, token, 0, NULL, 0);
	if(reti) {
		//printf("There is an error in your entry\n");
		return 0;
	}
	return 1;
}

int isValidCombination(char** tokens, int numTokens) {
	int i;
	for(i=0; i<numTokens; i++) {
		if(!(isValidWord(tokens[i]) || isOperator(tokens[i]))) {
			return 0;
		}
		if(i==numTokens || i==0) {
			if(isOperator(tokens[i])) {
				return 0;
			}
		}
		/*
		if(isOperator(tokens[i]) && isOperator(tokens[i+1])) {
			return 0;
		}
		*/
	}
	return 1;
}

void forkAndHandle(char** command, char** args) {

	int p = fork();
	//printf("%s\n",command);
	if (p==0) {
		//printf("I'm the child");
		execve(*command, args, NULL);
	}
	else {
		int code;
		WEXITSTATUS(code);
		waitpid(p, &code, 0);
	}
}


void createTokenGroups(struct TokenGroupInfo* tokenGroups, int* lastGroupIndex, char** tokens, int numTokens) {

	tokenGroups[0].command = tokens[0];
	tokenGroups[0].args = (char**)malloc(50*sizeof(char*));
	memset(tokenGroups[0].args,0,50*sizeof(char*));
	
	// i iterates through tokens, 
	// j is index for tokenGroup number
	// k is index for argument number
	int i; int j=0; int k=0;
	int processingArguments = 1;

	for(int i = 0; i<numTokens; i++) {
		
		if(strcmp(tokens[i], "|") == 0) {	
			processingArguments = 1;
			tokenGroups[j+1].command = tokens[i+1];
			//printf("%d %s\n",j+1,*(tokenGroups[j+1].command));
			j++;
			tokenGroups[j].args = (char**)malloc(50*sizeof(char*));
			memset(tokenGroups[j].args,0,50*sizeof(char*));
			k=0;
		}

		else {
			if(processingArguments) {
				// append to arguments to argument array
				if(tokens[i+1]) {
					if(isOperator(tokens[i+1])) {
					  // if the next thing is an operator
						processingArguments = 0;
					}
				}	
				tokenGroups[j].args[k] = tokens[i];
				//printf("%s %d %s", "arg", k, ": ");
				//printf("%s\n", tokenGroups[j].args[k]);
				k++;
			}
			if(strcmp(tokens[i], "<") == 0) {
				tokenGroups[j].fileSpecIn = tokens[i+1];
				printf("%s %d\n", tokenGroups[j].fileSpecIn, j);
			}	
			else if(strcmp(tokens[i], ">") == 0)
				tokenGroups[j].fileSpecOut = tokens[i+1];
		}
	}
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
		if(i==0) {
			// First group - could have redirect in 
			if(tokenGroups[i].fileSpecOut) {
				printf("%s\n", "invalid input");
				//fprintf(stderr, "%d\n", );
			}
		}
		if(i==lastGroupIndex) {
			// Last group - could have redirect out
			if(tokenGroups[i].fileSpecIn) {
				printf("%s\n", "invalid input");
				//fprintf(stderr, "%d\n", );
			}
		}
		else { 
			// Neither first nor last group - no redirects
			if(tokenGroups[i].fileSpecIn || tokenGroups[i].fileSpecOut) {	
				printf("%s\n", "invalid input");
				//fprintf(stderr, "%d\n", );
			}
		}
	}
}

void createPipes(struct TokenGroupInfo* tokenGroups, int lastGroupIndex) {
	int i;
	int buffer[2];
	printf("----CREATING PIPES----\n");
	printf("HIGHEST TOKEN GROUP INDEX: %d\n", lastGroupIndex);

	// SPECIAL CASE - FIRST GROUP READ
	if (tokenGroups[0].fileSpecIn)
		tokenGroups[0].fds[0] = open(tokenGroups[0].fileSpecIn, O_RDONLY);
	else
		tokenGroups[0].fds[0] = -1;
	printf("  tokenGroups[0].fds[0] = %d\n", tokenGroups[0].fds[0]);	
	
	//
	for (i=0; i<lastGroupIndex; i++) {
		pipe(buffer);
		tokenGroups[i].fds[1] = buffer[1]; 			// write
		tokenGroups[i+1].fds[0] = buffer[0];		// read
		
		printf("    tokenGroups[%d].fds[1] = %d\n", i, tokenGroups[i].fds[1]);
		printf("  tokenGroups[%d].fds[0] = %d\n", i+1, tokenGroups[i+1].fds[0]);
	}

	// SPECIAL CASE - LAST GROUP WRITE
	if (tokenGroups[lastGroupIndex].fileSpecOut)
		tokenGroups[lastGroupIndex].fds[1] = open(tokenGroups[lastGroupIndex].fileSpecOut, O_WRONLY | O_CREAT | O_TRUNC, 00664);
	else
		tokenGroups[lastGroupIndex].fds[1] = -1;
	printf("    tokenGroups[%d].fds[1] = %d\n", lastGroupIndex, tokenGroups[lastGroupIndex].fds[1]);	
}


void printPipes(struct TokenGroupInfo* tokenGroups, int lastGroupIndex) {
	int i;
	printf("----CREATING PIPES----\n");
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
	int i; int j;

	//TokenGroupInfo* t = (TokenGroupInfo*)(malloc(sizeof(TokenGroupInfo)));
	for (i=0; i<=lastGroupIndex; i++) {
		int p = fork();
		if (p==0) {
			// THIS IS THE CHILD
			//printf("%s\n", "around l. 213");
			if (tokenGroups[i].fds[0] > -1) 
				dup2(tokenGroups[i].fds[0], STDIN_FILENO);
			if (tokenGroups[i].fds[1] > -1) 
				dup2(tokenGroups[i].fds[1], STDOUT_FILENO);
			
			for (j=0; j<lastGroupIndex; j++) {
				// close all other pipes
				if (j!=i) {
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
			
			if(tokenGroups[i].fds[0] > -1)
				close(tokenGroups[i].fds[0]);
			if(tokenGroups[i].fds[1] > -1)
				close(tokenGroups[i].fds[1]);
			int status;
			waitpid(p, &status, 0);
		}
	}
}

int main() {
  while(1) {
	//while(!feof(stdin))
    // make room for 100 characters for each thing user enters
    char* input = (char*)(malloc(101 * sizeof(char))); 
    printf("$ ");
    fgets(input, 101, stdin);
    input[strcspn(input, "\n")] = 0;
    
		// check for exit string
		if(strcmp("exit", input)==0)
			exit(0);

		int* pids = (int*)(malloc(50*sizeof(int)));
		char** tokens;
		tokens = malloc(101 * (sizeof(char*)));
		memset(tokens,0, 101 * sizeof(char*));

		char* token = strtok(input, " ");
		int numTokens = 0;
		createTokenArray(token, tokens, &numTokens);
		
		if(!isValidCombination(tokens, numTokens)) {
			printf("invalid input\n");
		}
		else {
			struct TokenGroupInfo* tokenGroups;
			tokenGroups = (struct TokenGroupInfo*)(malloc(20 * (sizeof(struct TokenGroupInfo))));
			int lastGroupIndex = 0;
			//printf("%s\n", "CREATING TOKENGROUPS");
			createTokenGroups(tokenGroups, &lastGroupIndex, tokens, numTokens);
			//printf("%s\n", "END CREATING TOKENGROUPS");
			checkGroups(tokenGroups, lastGroupIndex);
			printTokenGroups(tokenGroups, lastGroupIndex);		
			createPipes(tokenGroups, lastGroupIndex);
			printPipes(tokenGroups, lastGroupIndex);
			handleTokenGroups(tokenGroups, lastGroupIndex);
		}	
  }
  return 0;
}
