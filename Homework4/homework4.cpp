/*
* Steven Jenny
* 12/5/2016
* CS 4414
* Machine Problem 4 - FTP Server
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

int createSocketFD(int portNum, int ControlFD) {
  /* Create a new socket file descriptor in response to PORT */
	// structure for handling address
	struct sockaddr_in sa;
	// create file descriptor
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int res;

	// If failure to create socket print an error
  if (SocketFD == -1) {
    perror("cannot create socket");
		write(ControlFD, "451\r\n", 5);
    exit(EXIT_FAILURE);
  }
	// clear socket address sa
  memset(&sa, 0, sizeof sa);

	// set properties of sa included hardcoded ip address - code from Wikipedia
  sa.sin_family = AF_INET;
  sa.sin_port = htons(portNum);
  res = inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);
  
	// print error if failure to connect
	if (connect(SocketFD, (struct sockaddr *)&sa, sizeof sa) == -1) {
    perror("connect failed");
		write(ControlFD, "451\r\n", 5);
    close(SocketFD);
    exit(EXIT_FAILURE);
  }
	// return the new file descriptor
	return SocketFD;
}

void sendList(int SocketFD) {
	/* Send list of files including filename and file size */
	
	// create a file that holds the output from ls -l and read it via ifstream
	system("ls -al > /tmp/.listdirectory");
	ifstream infile("/tmp/.listdirectory");
	string line;

	// process output of ls (in file) line by line
	while(getline(infile, line)) {
		// parse columns by creating object and parsing with strtok
		char** columns = new char*[100 * sizeof(char*)];
		memset(columns, 0, 100 * sizeof(char*));
    // parse first entry
		char* col = strtok((char*)line.c_str(), " ");
    int numColumns;
		// parse every entry based on " " delimiter in between
    createTokenArray(col, columns, &numColumns, " ");
		// create newline string which will be constructed
		string newLine = "";
		// check for null characters
		if((columns[4]!=NULL) & (columns[8]!=NULL)) {
			// concatenate newline based on filename and filesize; write it to client FD
			newLine += string(columns[8]) + "\t" + string(columns[4]) + "\r\n";
			//cout << newLine << endl;
			write(SocketFD, newLine.c_str(), newLine.length());
		}
		// clean up
		delete[] columns;
	}
	// close ifstream file, data socket fd, and delete file that held contents of ls
	infile.close();
  close(SocketFD);
	system("rm -rf /tmp/.listdirectory");
}


void sendFile(FILE* systemFile, int ClientSockFD) {
	/* send file over data connection - used in response to a RECV */
	// get size of system file by seeking to end and noting where this is
	fseek(systemFile, 0L, SEEK_END);
	unsigned int systemFileSize = ftell(systemFile);
	// set arbitrary size buffer to send file in increments
	int blockSize = 4096;
	char buff[blockSize];
	// seek back to beginning of file for reading
	fseek(systemFile, 0, SEEK_SET);

	int i;
	// read in chunks except for last chunk
	for(i=0; (unsigned int)i<((systemFileSize-1)/blockSize); i++) {
		fread(buff, sizeof(char), blockSize, systemFile);
		//cout << ftell(systemFile) << endl;
		write(ClientSockFD,  buff, blockSize);
	}

	// if last chunk is exactly of size blocksize, write block-sized final chunk
	if(systemFileSize % blockSize == 0) {
		fread(buff, sizeof(char), blockSize, systemFile);
		//cout << ftell(systemFile) << endl;
		write(ClientSockFD,  buff, blockSize);
	}
	// otherwise write remainder, found by the % operator
	else {
		// create new buffer of remainder size, read that size, and write over data FD
		char buff2[systemFileSize % blockSize];
		fread(buff2, sizeof(char), (systemFileSize % blockSize), systemFile);
		write(ClientSockFD, buff2, (systemFileSize % blockSize));
	}
	// close the data connection
  close(ClientSockFD);
}

void writeFile(FILE* systemFile, int ClientSockFD) {
	/* write a file from the client to the server side, given open file and data connection FD */
	// set arbitrary size buffer to send file in increments
	int blockSize = 4096;
	char buff[blockSize];
	// seek back to beginning of file for reading
	fseek(systemFile, 0, SEEK_SET);
	
	// read numBytesRead bytes - this variable will be of blockSize unless the buffer is not being filled
	int numBytesRead = read(ClientSockFD, buff, blockSize);
	// write to system file
	fwrite(buff, 1, numBytesRead, systemFile);

	// while buffer is being filled, keep writing whatever amount is being placed into buffer
	while(numBytesRead == 4096) {
		numBytesRead = read(ClientSockFD, buff, blockSize);
		fwrite(buff, 1, numBytesRead, systemFile);
	}
	// close the data connection
  close(ClientSockFD);
}

int main( int argc, char *argv[] )
{
	int controlPort = atoi(argv[1]);
	// cout << "port: " << controlPort << endl;
	// structure for handling address
  struct sockaddr_in sa;
	// create control connection FD
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	// variables to guarantee correct mode and types
	bool binaryType = false;
	bool fileMode = true;
	// integer to hold FD of data connection if open
	int ClientSockFD;

	// buffer for holding input from client
	char buff[1000];
  if (SocketFD == -1) {
    perror("cannot create socket");
    exit(EXIT_FAILURE);
  }

	// clear socket address structure
  memset(&sa, 0, sizeof sa);

	// set fields of address variable including hardcoding port number
  sa.sin_family = AF_INET;
  sa.sin_port = htons(controlPort);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind to contol connection; print error in case of failure
  if (bind(SocketFD,(struct sockaddr *)&sa, sizeof sa) == -1) {
    perror("bind failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }
	
	// listen on control connection FD
  if (listen(SocketFD, 10) == -1) {
    perror("listen failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  for (;;) {
		// accept input over control connection
    int ConnectFD = accept(SocketFD, NULL, NULL);
		
		// handle error
    if (0 > ConnectFD) {
      perror("accept failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
		else {
			// accept connection
			write(ConnectFD, "220\r\n", 5); 
		}

		// continually process input until QUIT received
		while(1) {
			memset(buff, 0, 100 * sizeof(char));
			read(ConnectFD, buff, 100);
			// The following line was used heavily in testing to check what was received
			//cout << "client>: " << buff << "||" << endl;
			
			/*
				These commands needed to be implemented for basic implementation:
			 	USER, QUIT, PORT, TYPE, MODE, STRU,
			 	for the default values
			 	RETR, STOR, NOOP.
				(note: LIST was also implemented)
			*/

			if(strncmp("USER", buff, 4)==0) {
				/*
					Set user
				*/
				// accept all users
				write(ConnectFD, "230\r\n", 5);
			}
			
			// exit cleanly in response to a QUIT
			else if(strncmp("QUIT", buff, 4)==0) {
				write(ConnectFD, "221\r\n", 5);
				break;
			}

			else if(strncmp("PORT", buff, 4)==0) {
				/*
					Open new port - used for data connection in ls, get, put
				*/
				// tokenize input based on " " delimiter
				char** tokens = new char*[101 * sizeof(char*)];
    		char* token = strtok(buff, " ");
    		int numTokens;
    		createTokenArray(token, tokens, &numTokens, " ");
				
				// only works if two arguments given
				if(numTokens == 2) {	
					// tokenize IP address on "," delimiter
					char** ipNums = new char*[101 * sizeof(char*)];
    			char* firstNum = strtok(tokens[1], ",");
    			int numIpNums;
    			createTokenArray(firstNum, ipNums, &numIpNums, ",");
					
					// calculate port based on <second to last int> * 256 + <last int>
					int ftp_data_port = atoi(ipNums[numIpNums-2]) * 256 + atoi(ipNums[numIpNums-1]);
					
					// open connection for data
					ClientSockFD = createSocketFD(ftp_data_port, ConnectFD);
					// success if made it to here - clean up
					write(ConnectFD, "200\r\n", 5);
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
					Switch type between binary and ascii
				*/
				// tokenize input on " " delimiter
				char** tokens = new char*[101 * sizeof(char*)];
    		char* token = strtok(buff, " ");
    		int numTokens;
    		createTokenArray(token, tokens, &numTokens, " ");

				// must have 2 arguments
				if(numTokens == 2) {	
					write(ConnectFD, "200\r\n", 5);
					// if image type specified, set binary type to true
					if(strncmp(tokens[1],"I", 1)==0) {
						binaryType = true;
					}
					else {
						binaryType = false;
					}
				}
				// wrong number of arguments
				else {
					cout << "wrong number of args!" << endl;
					write(ConnectFD, "501\r\n", 5);
				}
				// garbage collection
				delete[] tokens;
			}

			else if(strncmp("MODE", buff, 4)==0) {
		  	/*
					Switch mode between Stream and other (other modes fail - send 504)
				*/
				// tokenize input on delimiter " "
				char** tokens = new char*[101 * sizeof(char*)];
    		char* token = strtok(buff, " ");
    		int numTokens;
    		createTokenArray(token, tokens, &numTokens, " ");

				// must have 2 arguments
				if(numTokens == 2) {
					// if stream mode, success, otherwise error
					if(strncmp(tokens[1], "S\r\n", 3)==0) {
						write(ConnectFD, "200\r\n", 5);	
					}
					else {
						write(ConnectFD, "504\r\n", 5);
					}
				}
				// wrong number of arguments
				else {
					cout << "wrong number of args!" << endl;
					write(ConnectFD, "501\r\n", 5);
				}
				// garbage collection
				delete[] tokens;
			}

			else if(strncmp("STRU", buff, 4)==0) {
		  	/*
					Specify file structure - only F (file) is valid
				*/
				// tokenize input on delimiter " "
				char** tokens = new char*[101 * sizeof(char*)];
    		char* token = strtok(buff, " ");
    		int numTokens;
    		createTokenArray(token, tokens, &numTokens, " ");

				// must have 2 arguments
				if(numTokens == 2) {
					// if stream mode, success, otherwise error
					if(strncmp(tokens[1], "F\r\n", 3)==0) {
						write(ConnectFD, "200\r\n", 5);
					}
					else {
						write(ConnectFD, "504\r\n", 5);
					}
				}
				// wrong number of arguments
				else {
					cout << "too many args!" << endl;
					write(ConnectFD, "501\r\n", 5);
				}
				// garbage collection
				delete[] tokens;
			}

			else if(strncmp("LIST", buff, 4)==0) {
			  /*
					Send LIST (ls -la) of files on server side.
				*/
				// set binaryType flag to false since ls switches type to ascii
				binaryType = false;
				// data connection open, about to send
				write(ConnectFD, "125\r\n", 5);
				// send file list over data connection
				sendList(ClientSockFD);
				// closing data connection; requested file successful
				write(ConnectFD, "226\r\n", 5);	
			}

			else if(strncmp("RETR", buff, 4)==0) {
				/*
					Retrieve file from server
				*/
				// if not in binary mode don't do it; send error code
				if(!binaryType) {
					write(ConnectFD, "503\r\n", 5);
				}
				// in binary mode - proceed
				else {
					// tokenize input on delimiter " "
    			char** args = new char*[3 * sizeof(char*)];
    			char* arg1 = strtok(buff, " ");
    			int numArgs;
    			createTokenArray(arg1, args, &numArgs, " ");
   
    			// file to be read - remove carriage return
					string filename = string(args[1]);
					filename.erase(filename.rfind('\r'));
					// open file for reading
    			FILE* systemFile = fopen(filename.c_str(), "r+b");
					// fail if file not there
					if(systemFile == NULL) {
						write(ConnectFD, "550\r\n", 5);
					}
					else {
						// data connection already open; transfer starting
						write(ConnectFD, "125\r\n", 5);
						// send over open data connection
						sendFile(systemFile, ClientSockFD);
						// Closing data connection - requested file action successful
						write(ConnectFD, "226\r\n", 5);	
						// close system file
						fclose(systemFile);
					}
				}
			}

			else if(strncmp("STOR", buff, 4)==0) {
				/*
					Store file from client on server
				*/
				// if not in binary mode don't do it; send error code
				if(!binaryType) {
					write(ConnectFD, "503\r\n", 5);
				}
				// in binary mode - proceed
				else {
					// tokenize input on delimiter " "
    			char** args = new char*[3 * sizeof(char*)];
    			char* arg1 = strtok(buff, " ");
    			int numArgs;
    			createTokenArray(arg1, args, &numArgs, " ");
   
    			// file to be read - remove carriage return
					string filename = string(args[1]);
					filename.erase(filename.rfind('\r'));
					// open file for writing
    			FILE* systemFile = fopen(filename.c_str(), "wb");
					// fail if file not found
					if(systemFile == NULL) {
						write(ConnectFD, "550\r\n", 5);
					}
					else {
						// data connection already open; transfer starting
						write(ConnectFD, "125\r\n", 5);
						// write over open data connection
						writeFile(systemFile, ClientSockFD);
						// Closing data connection - requested file action successful
						write(ConnectFD, "226\r\n", 5);	
						// close system file
						fclose(systemFile);
					}
				}
			}

			else if(strncmp("NOOP", buff, 4)==0) {
				/*
				 Do nothing - send 200
				*/
				write(ConnectFD, "200\r\n", 5); 
			}
			else {
				// otherwise through command not recognized error
				write(ConnectFD, "500\r\n", 5); 	
			}
		}
		// shutdown and handle case of failure
    if (shutdown(ConnectFD, SHUT_RDWR) == -1) {
    	perror("shutdown failed");
    	close(ConnectFD);
    	close(SocketFD);
    	exit(EXIT_FAILURE);
    }
		// close the control connection
    close(ConnectFD);
		break;
  }
	// close the socket
  close(SocketFD);
  return EXIT_SUCCESS;  
}
