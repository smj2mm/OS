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
	char** address;
	char** args;
	char* fileSpecOut;
	char* fileSpecIn;
	char* afterOperator;
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
		int status;
		waitpid(p, &status, 0);
	}
}


void createTokenGroups(struct TokenGroupInfo* tokenGroupAddresses, int* numTokenGroups, char** tokens, int numTokens) {

	tokenGroupAddresses[0].address = &tokens[0];
	tokenGroupAddresses[0].args = (char**)malloc(50*sizeof(char*));
	memset(tokenGroupAddresses[0].args,0,50*sizeof(char*));
	
	// i iterates through tokens, 
	// j is index for tokenGroup number
	// k is index for argument number
	int i; int j=0; int k=0;
	int processingArguments = 1;

	for(int i = 0; i<numTokens; i++) {
		
		if(strcmp(tokens[i], "|") == 0) {	
			processingArguments = 1;
			tokenGroupAddresses[j].address = &tokens[i+1];
			printf("%s\n",*(tokenGroupAddresses[j].address));
			j++;
			tokenGroupAddresses[j].args = (char**)malloc(50*sizeof(char*));
			memset(tokenGroupAddresses[j].args,0,50*sizeof(char*));
			k=0;
		}

		else {
			if(processingArguments) {
				// append to arguments to argument array
				if(!tokens[i+1]) {
					;;
				}
				else if(isOperator(tokens[i+1])) {
					// if the next thing is an operator
					processingArguments = 0;
				}
				tokenGroupAddresses[j].args[k] = tokens[i];
				printf("%s %d %s", "arg", k, ": ");
				printf("%s\n", tokenGroupAddresses[j].args[k]);
				k++;
			}
			if(strcmp(tokens[i], "<") == 0) {
				tokenGroupAddresses[j].fileSpecIn = tokens[i+1];
				printf("%s %d\n", tokenGroupAddresses[j].fileSpecIn, j);
			}	
			else if(strcmp(tokens[i], ">") == 0)
				tokenGroupAddresses[j].fileSpecOut = tokens[i+1];
		}
	}
	*numTokenGroups = j;
}

void redirectIn(struct TokenGroupInfo tokenGroupAddress) {
	// Handle '<' operators
	int p = fork();
	if(p==0) {
		printf("I AM THE CHILD %s\n", tokenGroupAddress.fileSpecIn);
		int fd = open(tokenGroupAddress.fileSpecIn, O_RDONLY);
		dup2(fd, STDIN_FILENO);
		execve(*(tokenGroupAddress.address), tokenGroupAddress.args, NULL);
	}
	else {
		;;
	}
}

void redirectOut(struct TokenGroupInfo tokenGroupAddress) {
	// Handle '>' operators
	int p = fork();
	if(p==0) {
		// If this is the child process
		// 00664 = ... -wx -wx r--
		int fd = open(tokenGroupAddress.fileSpecOut, O_WRONLY | O_CREAT | O_TRUNC, 00664);
		dup2(fd, STDOUT_FILENO);
		execve(*(tokenGroupAddress.address), tokenGroupAddress.args, NULL);
	}
	else {
		;;
	}
}

void createPipes(struct TokenGroupInfo* tokenGroupAddresses, int numTokenGroups) {
	int fds[2];
	int i;
	/*
	for(i=0; i<numTokenGroups; i++) {
		// Make number of groups - 1 pipes
		if(i==0) {
			// First group - could have redirect in 
			if(tokenGroupAddresses[i].fileSpecIn) {
				redirectIn(tokenGroupAddresses);
			}
		}
		if(i==numTokenGroups-1) {
			// Last group - could have redirect out
			if(tokenGroupAddresses[i]
		}
		else { 
			// Neither first nor last group - no redirects
		}
	}*/
}

void handleTokenGroups(struct TokenGroupInfo* tokenGroupAddresses, int numTokenGroups) {	
	int i, j;

	//createPipes(tokenGroupAddresses, numTokenGroups);

	//TokenGroupInfo* t = (TokenGroupInfo*)(malloc(sizeof(TokenGroupInfo)));
	for(i=0; i<=numTokenGroups; i++) {
		j=0; char * iterator = "";
		if(tokenGroupAddresses[i].fileSpecIn) {
			redirectIn(tokenGroupAddresses[i]);
		}

		if(tokenGroupAddresses[i].fileSpecOut) {
			redirectOut(tokenGroupAddresses[i]);
		}

		//forkAndHandle(tokenGroupAddresses[i].address, tokenGroupAddresses[i].args);
	}
}

int main() {
  while(1) {
    // make room for 100 characters for each thing user enters
    char* input = (char*)(malloc(101 * sizeof(char))); 
    printf("$ ");
    fgets(input, 101, stdin);
    input[strcspn(input, "\n")] = 0;
    
		// check for exit string
		if(strcmp("exit", input)==0)
			exit(0);

		char** tokens;
		tokens = malloc(101 * (sizeof(char*)));
		memset(tokens,0, 101 * sizeof(char*));

		char* token = strtok(input, " ");
		int numTokens = 0;
		createTokenArray(token, tokens, &numTokens);
		
		if(!isValidCombination(tokens, numTokens)) {
			printf("Error in entry\n");
		}
		else {
			;;
			//forkAndHandle(&tokens[0]);
		}
		
		struct TokenGroupInfo* tokenGroupAddresses;
		tokenGroupAddresses = (struct TokenGroupInfo*)(malloc(20 * (sizeof(struct TokenGroupInfo))));
		int numTokenGroups = 0;
		//printf("%s\n", "CREATING TOKENGROUPS");
		createTokenGroups(tokenGroupAddresses, &numTokenGroups, tokens, numTokens);
		//printf("%s\n", "END CREATING TOKENGROUPS");
		handleTokenGroups(tokenGroupAddresses, numTokenGroups);
		int i;
		/*
		for(i=0; i<=numTokenGroups; i++) {
			printf("%d\n", i);
			//printf("%s", *(tokenGroupAddresses[i].address));
			forkAndHandle(tokenGroupAddresses[i].address, tokenGroupAddresses[i].args);
		}*/
  }
  return 0;
}
