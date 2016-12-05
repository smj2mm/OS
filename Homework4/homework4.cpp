/*
* Steven Jenny
* 9/29/2016
* CS 4414
* Machine Problem 2 - Finding the Maximum Value
*/

#include <unistd.h>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using namespace std;

/* CLIENT STEPS
* 1. Create a socket with the socket() system call
* 2. Connect the socket to the address of the server using the connect() 
*				system call
* 3. Send and receive data. There are a number of ways to do this, but 
*				the simplest is to use the read() and write() system calls.
*/


/* SERVER STEPS
* 1. Create a socket with the socket() system call
* 2. Bind the socket to an address using the bind() system call. For a 
				server socket on the Internet, an address consists of a port 
				number on the host machine.
* 3. Listen for connections with the listen() system call
* 4. Accept a connection with the accept() system call. This call 
				typically blocks until a client connects with the server.
* 5. Send and receive data
*/

void createTokenArray(char* token, char** tokens, int* numTokens, const char delimiter[2]) {
  // tokenize a string based on the delimiter object
  int i = 0;
  while(token != NULL) {
    // create space, add token
    tokens[i] = (char*) malloc(sizeof(token));
    tokens[i] = token;
    // tokenize based on the passed in delimiter
    token = strtok(NULL, delimiter);
    // check for null token
    if(token!=NULL) {
      // remove newline character if it is present
      char* endOfToken = token+strlen(token)-1;
      if(strncmp(endOfToken,"\n",1)==0) {
        *remove(token, endOfToken, '\n') = '\0';
      }   
    }   
    i++;
    // update number of tokens
    *numTokens=i;
  }
}

string addSpaces(char* word) {
	int numberOfSpaces = 8 - strlen(word);
	string returnString = "";
	if(numberOfSpaces <= 0);
	else {
		int i;
		for(i=0; i<numberOfSpaces; i++) {
			returnString += " ";
		}
	}
	return returnString;
}

int createSocketFD(int portNum) {
  struct sockaddr_in sa;
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int res;

	char buff[100];
  if (SocketFD == -1) {
    perror("cannot create socket");
    exit(EXIT_FAILURE);
  }

  memset(&sa, 0, sizeof sa);

  sa.sin_family = AF_INET;
  sa.sin_port = htons(portNum);
 	//inet_aton("127.0.0.1", &sa.sin_addr);
  res = inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);
  
	if (connect(SocketFD, (struct sockaddr *)&sa, sizeof sa) == -1) {
    perror("connect failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }
	return SocketFD;
}

void sendList(int SocketFD) {
	//system("bash ls -l | awk '{print $5 $9}'");
	system("ls -l > .listdirectory");
	ifstream infile(".listdirectory");
	string line;

	while(getline(infile, line)) {
		char** columns = new char*[100 * sizeof(char*)];
		memset(columns, 0, 100 * sizeof(char*));
    char* col = strtok((char*)line.c_str(), " ");
    int numColumns;
    createTokenArray(col, columns, &numColumns, " ");		
		string newLine = "";
		if(columns[4]!=NULL & columns[8]!=NULL) {
			//string spaces = addSpaces(columns[4]);
			newLine += string(columns[8]) + "\t" + string(columns[4]) + "\r\n";
			//fwrite(newLine.c_str(), newLine.length(), 1, systemFile); 
			cout << newLine << endl;
			write(SocketFD, newLine.c_str(), newLine.length());
		}
		delete[] columns;
	}
	infile.close();
  close(SocketFD);
	system("rm -rf .listdirectory");
}


void sendFile(FILE* systemFile, int ClientSockFD) {
	// get size of system file by seeking to end and noting where this is
	fseek(systemFile, 0L, SEEK_END);
	unsigned int systemFileSize = ftell(systemFile);
	int blockSize = 4096;
	char buff[blockSize];
	fseek(systemFile, 0, SEEK_SET);

	int i;
	for(i=0; i<((systemFileSize-1)/blockSize); i++) {
		fread(buff, sizeof(char), blockSize, systemFile);
		cout << ftell(systemFile) << endl;
		write(ClientSockFD,  buff, blockSize);
	}

	if(systemFileSize % blockSize == 0) {
		fread(buff, sizeof(char), blockSize, systemFile);
		cout << ftell(systemFile) << endl;
		write(ClientSockFD,  buff, blockSize);
	}
	else {
		char buff2[systemFileSize % blockSize];
		fread(buff2, sizeof(char), (systemFileSize % blockSize), systemFile);
		write(ClientSockFD, buff2, (systemFileSize % blockSize));
	}
  close(ClientSockFD);
}

void writeFile(FILE* systemFile, int ClientSockFD) {
	// get size of system file by seeking to end and noting where this is
	int blockSize = 4096;
	char buff[blockSize];
	fseek(systemFile, 0, SEEK_SET);
	
	int numBytesRead = read(ClientSockFD, buff, blockSize);
	fwrite(buff, 1, numBytesRead, systemFile);

	while(numBytesRead == 4096) {
		numBytesRead = read(ClientSockFD, buff, blockSize);
		fwrite(buff, 1, numBytesRead, systemFile);
	}
  close(ClientSockFD);
}

int main(void)
{
	//sendList();
  struct sockaddr_in sa;
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	bool binaryMode = false;
	bool fileMode = true;
	int ClientSockFD;

	char buff[100];
  if (SocketFD == -1) {
    perror("cannot create socket");
    exit(EXIT_FAILURE);
  }

  memset(&sa, 0, sizeof sa);

  sa.sin_family = AF_INET;
  sa.sin_port = htons(1100);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(SocketFD,(struct sockaddr *)&sa, sizeof sa) == -1) {
    perror("bind failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  if (listen(SocketFD, 10) == -1) {
    perror("listen failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

	unsigned int ftp_data_port = 20;

  for (;;) {
    int ConnectFD = accept(SocketFD, NULL, NULL);
			
    if (0 > ConnectFD) {
      perror("accept failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
		else {
			// accept connection
			write(ConnectFD, "220\r\n", 5); 
		}

		while(1) {
			memset(buff, 0, 100 * sizeof(char));
			read(ConnectFD, buff, 100);
			cout << "client>: " << buff << "||" << endl;
			// read, parse, and do whatever needed for that command
			
			/*
			 USER, QUIT, PORT, TYPE, MODE, STRU,
			 for the default values
			 RETR, STOR, NOOP.
			*/
			if(strncmp("USER", buff, 4)==0) {
				/* 
				 230
			   530
				 500, 501, 421
				 331, 332
				*/
				write(ConnectFD, "230\r\n", 5);
			}

			else if(strncmp("QUIT", buff, 4)==0) {
				write(ConnectFD, "221\r\n", 5);
				break;
			}

			else if(strncmp("PORT", buff, 4)==0) {
				/*
				 200
				 500, 501, 421, 530
				*/
				char** tokens = new char*[101 * sizeof(char*)];
    		char* token = strtok(buff, " ");
    		int numTokens;
    		createTokenArray(token, tokens, &numTokens, " ");

				if(numTokens == 2) {
					write(ConnectFD, "200\r\n", 5);
					
					char** ipNums = new char*[101 * sizeof(char*)];
    			char* firstNum = strtok(tokens[1], ",");
    			int numIpNums;
    			createTokenArray(firstNum, ipNums, &numIpNums, ",");
					
					int ftp_data_port = atoi(ipNums[numIpNums-2]) * 256 + atoi(ipNums[numIpNums-1]);
					
					cout << "port number: " << ftp_data_port << endl;
					ClientSockFD = createSocketFD(ftp_data_port);
					cout << "ftp data port changed to: " << ftp_data_port << endl;
					delete[] ipNums;
				}
				else {
					cout << "too many args!" << endl;
					write(ConnectFD, "501\r\n", 5);
				}
				delete[] tokens;
			}

			else if(strncmp("TYPE", buff, 4)==0) {
				/*
				 200
				 500, 501, 504, 421, 530
				*/
				char** tokens = new char*[101 * sizeof(char*)];
    		char* token = strtok(buff, " ");
    		int numTokens;
    		createTokenArray(token, tokens, &numTokens, " ");

				if(numTokens == 2) {	
					write(ConnectFD, "200\r\n", 5);
					if(strncmp(tokens[1],"I", 1)==0) {
						binaryMode = true;
						cout << "binaryMode is true!\n";
					}
					else {
						binaryMode = false;
					}
				}
				else {
					cout << "too many args!" << endl;
					write(ConnectFD, "501\r\n", 5);
				}
				delete[] tokens;
			}

			else if(strncmp("MODE", buff, 4)==0) {
		  	/*
				 200
      	 500, 501, 504, 421, 530
				*/
				char** tokens = new char*[101 * sizeof(char*)];
    		char* token = strtok(buff, " ");
    		int numTokens;
    		createTokenArray(token, tokens, &numTokens, " ");

				if(numTokens == 2) {
					if(strncmp(tokens[1], "S\r\n", 3)==0) {
						write(ConnectFD, "200\r\n", 5);	
					}
					else {
						write(ConnectFD, "504\r\n", 5);
					}
				}
				else {
					cout << "too many args!" << endl;
					write(ConnectFD, "501\r\n", 5);
				}
				delete[] tokens;
			}

			else if(strncmp("STRU", buff, 4)==0) {
		  	/*
				 200
      	 500, 501, 504, 421, 530
				*/
				char** tokens = new char*[101 * sizeof(char*)];
    		char* token = strtok(buff, " ");
    		int numTokens;
    		createTokenArray(token, tokens, &numTokens, " ");

				if(numTokens == 2) {
					if(strncmp(tokens[1], "F\r\n", 3)==0) {
						write(ConnectFD, "200\r\n", 5);
						fileMode = true;	
					}
					else {
						write(ConnectFD, "504\r\n", 5);
					}
				}
				else {
					cout << "too many args!" << endl;
					write(ConnectFD, "501\r\n", 5);
				}
				delete[] tokens;
			}

			else if(strncmp("LIST", buff, 4)==0) {
			  /*
				125, 150
        	226, 250
          425, 426, 451
        450
        500, 501, 502, 421, 530	
				*/
				write(ConnectFD, "125\r\n", 5);		
				sendList(ClientSockFD);
				write(ConnectFD, "226\r\n", 5);	
			}

			else if(strncmp("RETR", buff, 4)==0) {
				/*
				 125, 150
            (110)
            226, 250
            425, 426, 451
         450, 550
         500, 501, 421, 530
				*/
				if(!binaryMode) {
					cout << "Not TYPE BINARY!!!" << endl;
					write(ConnectFD, "503\r\n", 5);
				}
				else {
    			char** args = new char*[3 * sizeof(char*)];
    			char* arg1 = strtok(buff, " ");
    			int numArgs;
    			createTokenArray(arg1, args, &numArgs, " ");
   
    			// file to be read
					string filename = string(args[1]);
					filename.erase(filename.rfind('\r'));
    			FILE* systemFile = fopen(filename.c_str(), "r+b");
					cout << "filename: " << filename.c_str() << endl;
					
					write(ConnectFD, "125\r\n", 5);
					sendFile(systemFile, ClientSockFD);
					//write(ClientSockFD, "AAAAAAAA\r\n", 10);
					write(ConnectFD, "200\r\n", 5);	
					fclose(systemFile);
				}
			}

			else if(strncmp("STOR", buff, 4)==0) {
				/*
				 125, 150
            (110)
            226, 250
            425, 426, 451, 551, 552
         532, 450, 452, 553
         500, 501, 421, 530
				*/
				if(!binaryMode) {
					cout << "Not TYPE BINARY!!!" << endl;
					write(ConnectFD, "503\r\n", 5);
				}
				else {
    			char** args = new char*[3 * sizeof(char*)];
    			char* arg1 = strtok(buff, " ");
    			int numArgs;
    			createTokenArray(arg1, args, &numArgs, " ");
   
    			// file to be read
					string filename = string(args[1]);
					filename.erase(filename.rfind('\r'));
    			FILE* systemFile = fopen(filename.c_str(), "wb");
					cout << "filename: " << filename.c_str() << endl;
					
					write(ConnectFD, "125\r\n", 5);
					writeFile(systemFile, ClientSockFD);
					write(ConnectFD, "200\r\n", 5);	
					fclose(systemFile);
				}
			}

			else if(strncmp("NOOP", buff, 4)==0) {
				/*
				 200
         500 421
				*/
				write(ConnectFD, "200\r\n", 5); 
			}
			else {
				write(ConnectFD, "500\r\n", 5); 	
			}
		}
    if (shutdown(ConnectFD, SHUT_RDWR) == -1) {
    	perror("shutdown failed");
    	close(ConnectFD);
    	close(SocketFD);
    	exit(EXIT_FAILURE);
    }
    close(ConnectFD);
		break;
  }

  close(SocketFD);
  return EXIT_SUCCESS;  
}
