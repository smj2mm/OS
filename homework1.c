#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <regex.h>
#include <unistd.h>

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

void forkAndHandle(char** command) {

	int p = fork();
	//printf("%s\n",command);
	if (p==0) {
		//printf("I'm the child");
		execve(*command, command, NULL);
	}
	else {
		int status;
		waitpid(p, &status, 0);
	}
}

void createTokenGroups(char*** tokenGroups, int* numTokenGroups, char** tokens, int numTokens) {
	tokenGroups[0] = &tokens[0];
	int i; int j=1;
	for(i=1; i<numTokens; i++) {
		if(strcmp(tokens[i],"|") == 0) {
			//printf("%s\n", tokens[i+1]);
			tokenGroups[j] = &tokens[i+1];
			j++;
		}
	}
	*numTokenGroups = j;
}

void printTokenGroups(char *** tokenGroupAddresses, int numTokenGroups) {
	int i, j;
	for(i=0; i<numTokenGroups; i++) {
		j=0; char * iterator = "";
		printf("%s\n", "----------");
		if(i == numTokenGroups-1) {
			while(iterator) {
				printf("%s\n", *(tokenGroupAddresses[i]+j));
				iterator = *(tokenGroupAddresses[i]+j+1);
				j++;
			}
		}
		else {
			while(strcmp(iterator, "|")!=0) {
				printf("%s\n", *(tokenGroupAddresses[i]+j));
				iterator = *(tokenGroupAddresses[i]+j+1);
				j++;
			}
		}
	}
}

void handleTokenGroups(char *** tokenGroupAddresses, int numTokenGroups) {	
	int i, j;
	char** args = malloc(50 * sizeof(char*));
	memset(args, 0, 50*sizeof(char*));
	for(i=0; i<numTokenGroups; i++) {
		j=0; char * iterator = "";
		printf("%s\n", "----------");
		if(i == numTokenGroups-1) {
			while(iterator) {
				//printf("%s\n", *(tokenGroupAddresses[i]+j));
				args[j] = *(tokenGroupAddresses[i]+j);
				iterator = *(tokenGroupAddresses[i]+j+1);
				j++;
			}
		}
		else {
			while(strcmp(iterator, "|")!=0) {
				//printf("%s\n", *(tokenGroupAddresses[i]+j));
				args[j] = *(tokenGroupAddresses[i]+j);
				iterator = *(tokenGroupAddresses[i]+j+1);
				j++;
			}
		}
		forkAndHandle(tokenGroupAddresses[i]);
	}
}

int main() {
  while(1) {
    // make room for 100 characters for each thing user enters
    char* input = (char*)(malloc(101 * sizeof(char))); 
    printf("$ ");
    fgets(input, 101, stdin);
    input[strcspn(input, "\n")] = 0;
    //scanf("%100s", input);
    
		// check for exit string
		if(strcmp("exit", input)==0)
			exit(0);

		char** tokens;
		tokens = malloc(101 * (sizeof(char*)));

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

		char*** tokenGroupAddresses;
		tokenGroupAddresses = malloc(101 * (sizeof(char**)));
		int numTokenGroups = 0;
		//printf("%s\n", "CREATING TOKENGROUPS");
		createTokenGroups(tokenGroupAddresses, &numTokenGroups, tokens, numTokens);
		//printf("%s\n", "END CREATING TOKENGROUPS");

		printTokenGroups(tokenGroupAddresses, numTokenGroups);
		handleTokenGroups(tokenGroupAddresses, numTokenGroups);
  }
  return 0;
}
