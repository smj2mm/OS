#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <regex.h>
#include <unistd.h>

int main() {
	//char* ls = malloc(sizeof(char*));
	char* ls = "/bin/ls";

	execve(ls,&ls,NULL);
}
