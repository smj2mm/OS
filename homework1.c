#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
		
		int i = 0;
		while(token != NULL) {
			tokens[i] = malloc(sizeof(token));
			printf("%s \n", token);
			token = strtok(NULL, " ");
			i++;
      //printf("%s \n", input);
		}
  }
  return 0;
}
