/*
* Steven Jenny
* 11/20/2016
* CS 4414
* Machine Problem 3 - FAT File System
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <string.h>
#include <stack>  // std::stack
#include <algorithm>
#include <sys/stat.h>

// Macros for bitflags on attributes of a Fat16Entry
#define	READ_ONLY 		0x01
#define HIDDEN 				0x02
#define SYSTEM 				0x04
#define VOLUME_LABEL 	0x08
#define SUBDIRECTORY 	0x10
#define ARCHIVE 			0x20

using namespace std;

typedef struct {
  // Stores information about files in directories on the file system
	unsigned char filename[8];
  unsigned char ext[3];
  unsigned char attributes;
  unsigned char reserved[10];
  unsigned short modify_time;
  unsigned short modify_date;
  unsigned short starting_cluster;
	int32_t file_size;
} __attribute((packed)) Fat16Entry;

typedef struct {
	// Stores information about the information in the boot sector of the file system
  unsigned char jmp[3];
  char oem[8];
  unsigned short sector_size;
  unsigned char sectors_per_cluster;
  unsigned short reserved_sectors;
  unsigned char number_of_fats;
  unsigned short root_dir_entries;
  unsigned short total_sectors_short; // if zero, later field is used
  unsigned char media_descriptor;
  unsigned short fat_size_sectors;
  unsigned short sectors_per_track;
  unsigned short number_of_heads;
  unsigned long hidden_sectors;
  unsigned long total_sectors_long;
  
  unsigned char drive_number;
  unsigned char current_head;
  unsigned char boot_signature;
  unsigned long volume_id;
  char volume_label[11];
  char fs_type[8];
  char boot_code[448];
  unsigned short boot_sector_signature;
	// packed == ignore word alignment
} __attribute((packed)) Fat16BootSector;


void printBootSector(Fat16BootSector* b) {
	// Printing the boot sector - used in testing
  cout << "jmp: " << b->jmp << "\n";
  cout << "oem: " << b->oem << "\n";
  cout << "sector_size: " << b->sector_size << "\n"; // 2 bytes
  cout << "sectors_per_cluster: " << b-> sectors_per_cluster << "\n";
  cout << "reserved_sectors: " << b->reserved_sectors << "\n";
  cout << "number_of_fats: " << b->number_of_fats << "\n";
  cout << "root_dir_entries: " << b->root_dir_entries << "\n";
  cout << "total_sectors_short: " << b->total_sectors_short << "\n"; // if zero, later field is used
  cout << "media_descriptor: " << b->media_descriptor << "\n";
  cout << "fat_size_sectors: " << b->fat_size_sectors << "\n";
  cout << "sectors_per_track: " << b->sectors_per_track << "\n";
  cout << "number_of_heads: " << b->number_of_heads << "\n";
  cout << "hidden_sectors: " << b->hidden_sectors << "\n";
  cout << "total_sectors_long: " << b->total_sectors_long << "\n";
  
  cout << "drive_number: " << b->drive_number << "\n";
  cout << "current_head: " << b->current_head << "\n";
  cout << "boot_signature: " << b->boot_signature << "\n";
  cout << "volume_id: " << b->volume_id << "\n";
  cout << "volume_label: " << b->volume_label << "\n";
  cout << "fs_type: " << b->fs_type << "\n";
  cout << "boot_code: " << b->boot_code << "\n";
  cout << "boot_sector_signature: " << b->boot_sector_signature << "\n";	
}

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

bool checkEntry(Fat16Entry f) {
	// check whether a given directory entry is printable in LS. 
	// This will return true unless the file is hidden, is a system file, or is a volume
	if((f.attributes & HIDDEN) == HIDDEN) {
		return false;
	}
	if((f.attributes & SYSTEM) == SYSTEM) {
		return false;
	}
	if((f.attributes & VOLUME_LABEL) == VOLUME_LABEL) {
		return false;
	}
	return true;
}

void printDirEntry(Fat16Entry r) {
	// Printing the obects held in a directory entry; this was included for testing
  cout << "filename: " << r.filename << "; size: " << sizeof(r.filename) << "\n";
  cout << "ext: " << r.ext << "; size: " << sizeof(r.ext) << "\n";
  cout << "attributes: " << r.attributes << "; size: " << sizeof(r.attributes) << "\n";
  cout << "reserved: " << r.reserved << "; size: " << sizeof(r.reserved) << "\n";
  cout << "modify_time: " << r.modify_time << "; size: " << sizeof(r.modify_time) << "\n";
  cout << "modify_date: " << r.modify_date << "; size: " << sizeof(r.modify_date) << "\n";
  cout << "starting_cluster: " << r.starting_cluster << "; size: " << sizeof(r.starting_cluster) << "\n";
  cout << "file_size: " << r.file_size << "; size: " << sizeof(r.file_size) << "\n";	
}

void printDir(Fat16Entry* dir) {
	// Print out items in directory (for ls)
	int i=0;
	// while the filename is not empty
	while(dir[i].filename[0] != 0x00) {
		// if the directory fits the criteria for printing out
		if(checkEntry(dir[i])) {
			// if it's a subdirectory, only print the name
			int j;
			if((dir[i].attributes & SUBDIRECTORY) == SUBDIRECTORY) {
				for(j=0; j<8; j++) {
					if(dir[i].filename[j]== ' ') {
						break;
					}
					else {
						cout << dir[i].filename[j];
					}
					//printf ("%.8s\n", dir[i].filename);
				}
				cout << "\n";
			}
			else {
				// if it's not a subdirectory, print the filename and extension
				for(j=0; j<8; j++) {
					if(dir[i].filename[j] == ' ') {
						break;
					}
					else {
						cout << dir[i].filename[j];
					}
					//printf ("%.8s\n", dir[i].filename);
				}
				printf (".%.3s\n", dir[i].ext);
			}
		}
		i++;
	}
}

Fat16Entry* readInDir(int dirLoc, FILE* fatFile) {
	/* create a new Fat16Entry by reading from the FAT Volume at a particular location into a buffer */
	char buff[1000 * sizeof(Fat16Entry)];
	// create array of Fat16 entries
	Fat16Entry* dir = new Fat16Entry[1000];
	// seek to the directory location in FAT volume and read into the buffer
	fseek(fatFile, dirLoc, SEEK_SET);
	fread(buff, 1000 * sizeof(Fat16Entry), 1, fatFile);
	// cast the buffer and set it as the new directory
	dir = (Fat16Entry*) buff;
	
	return dir;
}

int computeRootDirLocation(Fat16BootSector* b) {
	/* computes the location of the root directory based on information in the boot sector*/
	return (b->reserved_sectors + (b->fat_size_sectors * b->number_of_fats)) * b->sector_size;
}

int computeDataLocation(Fat16BootSector* b) {
	/* computes the location of the beginning of the data region based on information in the boot sector*/
	return (b->reserved_sectors + (b->fat_size_sectors * b->number_of_fats) + sizeof(Fat16Entry)) * b->sector_size;
}

bool isDirectory(Fat16Entry f, FILE* fatFile) {
	/* returns true only if a given file is a subdirectory */
	if((f.attributes & SUBDIRECTORY) == SUBDIRECTORY) {
		return true;
	}
	return false;
}

unsigned int getFileLocation(int starting_cluster, FILE* fatFile, Fat16BootSector* b) {
	/* Given starting cluster, finds location in data region */
	if(starting_cluster == 0) {
		// special case for root
		return computeRootDirLocation(b);
	}
	// compute hex file location based off of information from the boot sector and the starting cluster
	int blocksToData = b->reserved_sectors + (b->fat_size_sectors * b->number_of_fats) + sizeof(Fat16Entry);
	return (((starting_cluster-2)*(b->sectors_per_cluster)) + blocksToData) * b->sector_size;
}

unsigned short getTableValue(int sector_size, int active_cluster, int first_fat_sector, FILE* fatFile) {
	/* get location of next location in FAT in chain */
	// create char array to hold linked data from the FAT 
	unsigned char FAT_table[sector_size * sizeof(Fat16Entry)];
  memset(FAT_table, 0, sector_size * sizeof(char));
	
	// calculate offset into the table based off of active cluster and the first cluster in the FAT
	unsigned int fat_offset = active_cluster * 2;
	unsigned int fat_sector = first_fat_sector + (fat_offset / sector_size);
	unsigned int ent_offset = fat_offset % sector_size;

	// at this point you need to read from sector "fat_sector" on the disk into "FAT_table".
	fseek(fatFile, fat_sector * sector_size, SEEK_SET);
	fread(FAT_table, sector_size, 1, fatFile);

	// get the next location in the table
	unsigned short table_value = *(unsigned short*)&FAT_table[ent_offset];
	return table_value; 
	// the variable "table_value" now has the information you need about the next cluster in the chain.
}

long findDir(char* name, FILE* fatFile, Fat16Entry* dir, Fat16BootSector* b) {
	/* find the location of a given subdirectory within a directory; if unsuccessful, return -1 */
	// do ls and compare filenames with name
	// if the name matches, check if it's a directory
	int i,j; char* currFilename = new char[8];
	//switch to while loop
	i=0;
	// Keep searching until the filename being considered is blank
	while(dir[i].filename[0] != 0x00) {
		j=0;
		while(j<8) {
			// Kill whitespace
			if(dir[i].filename[j] == ' ') {
				break;
			}
			// Add only the first 8 characters to the word to compare against the "name" passed into this function
			currFilename[j] = dir[i].filename[j];
			j++;
		}
		// if there is a match and it is a directory, find the hex location of that directory entry
		if(strncmp(currFilename, name, strlen(name))==0) {
			if(isDirectory(dir[i], fatFile)) {
				return getFileLocation(dir[i].starting_cluster, fatFile, b);
			}
		}
		i++;
	}
	// return -1 if unsuccessful
	return -1;
}

int findFileIndex(char* name, char* ext, FILE* fatFile, Fat16Entry* dir, Fat16BootSector* b) {	
	/* Return the index of a file within a given directory; if unsuccessful, return -1*/
	int i,j; char* currFilename = new char[8];
	i=0;
	// Keep searching until the filename being considered is blank
	while(dir[i].filename[0] != 0x00)	{
		j=0;	
		while(j<8) {
			// Kill whitespace
			if(dir[i].filename[j] == ' ') {
				break;
			}
			// Add only the first 8 characters to the word to compare against the "name" passed into this function
			currFilename[j] = dir[i].filename[j];
			j++;
		}

		// if there is a match in the filename and the extension matches, return the index within the directory
		if(strncmp(currFilename, name, strlen(name)-1)==0) {
			if(strncmp((char*)dir[i].ext, ext, 3)==0) {
				return i;
			}
		}
		i++;
	}
	return -1;
}

char* getFilenameAndExt(char **filename) {
	/* parse the filename, such that the extension is returned, and the original filename has the extension removed */
  char *lastDot = strrchr(*filename, '.');
	// if the function found an instance of '.', set the dot to '\0' for parsing reasons
  if(lastDot != NULL)
		*lastDot = '\0';
	// return the a pointer to the character right after the last dot (the extension)
	return lastDot + 1;
}

void copyOut(FILE* fatFile, Fat16Entry file, Fat16BootSector* b, FILE* systemFile) {
	/* The meat of cpout; writes a file from the Fat16 Volume to a specified system file */
	// create variables for the location to be used either in the case of small or large files and cluster size
	unsigned int locationInBytes;
	int cluster_size = b->sector_size * b->sectors_per_cluster;
	
	// if the file is smaller than or equal to the size of one cluster
	if(file.file_size <= cluster_size) {
		// create one buffer of the size of the file
		char* readBuff[file.file_size];
		
		// get the file location based on starting cluster in directory entry and write to that location
		locationInBytes = getFileLocation(file.starting_cluster, fatFile, b);
		fseek(fatFile, locationInBytes, SEEK_SET);
		fread(readBuff, 1, file.file_size, fatFile);
		fwrite(readBuff, 1, file.file_size, systemFile);
	}
	// if the file is larger than the size of one cluster
	else {
		// create one reusable buffer of the size of one cluster
		char* readBuff1[cluster_size];
		// create second buffer for remaining data in file after clusters
		char* readBuff2[file.file_size % cluster_size];
		// follow linked list based on nextStarting cluster - initialize to starting cluster of the new directory entry
		unsigned short nextStartingCluster = file.starting_cluster;
		
		while(nextStartingCluster < 0xFFF8) {
			// advance location based on previously found starting cluster
			locationInBytes = getFileLocation(nextStartingCluster, fatFile, b);
			// get next starting cluster
			nextStartingCluster = getTableValue(b->sector_size, nextStartingCluster, b->reserved_sectors, fatFile);	
			// if not on last cluster, write full cluster size
			if(nextStartingCluster < 0xFFF8) {
				fseek(fatFile, locationInBytes, SEEK_SET);
				fread(readBuff1, 1, cluster_size, fatFile);
				fwrite(readBuff1, 1, cluster_size, systemFile);
			}
			else {
				fseek(fatFile, locationInBytes, SEEK_SET);
				// if remaining is exactly the size of a cluster, write it all
				if(file.file_size % cluster_size == 0) {
					fread(readBuff1, 1, cluster_size, fatFile);
					fwrite(readBuff1, 1, cluster_size, systemFile);	
				}
				// otherwise only write remaining data, smaller than a cluster
				else {
					fread(readBuff2, 1, file.file_size % cluster_size, fatFile);
					fwrite(readBuff2, 1, file.file_size % cluster_size, systemFile);
				}
			}
		}
	}
}

void makeIntoEight(char* filename, char* newFileName) {
	/* Add spaces after the last letter in the name such that the filename contains a total of 8 characters */
	int i;
	bool stillLetters = true;

	// if the end of the char array has been reached and length isn't 8, add spaces after that point
	for(i=0; i<8; i++) {
		if((filename[i] != '\0') & (stillLetters)) {
			newFileName[i] = filename[i];
		}
		// add spaces from here to end by flipping boolean flag
		else {
			newFileName[i] = ' ';
			stillLetters = false;
		}
	}
}

unsigned int findOpenFatLoc(unsigned short* fat_table) {
	/* Finds the first unoccupied spot in the fat table */
	int j=0;
	while(fat_table[j] != 0) {
		j++;
	}
	return j;
}

Fat16Entry createFat16Entry(char* filename, char* ext, FILE* fatFile, unsigned int file_size, unsigned short startingCluster) {
	/* For cpin - create a new FAT16Entry with a given filename and extension */
	Fat16Entry newEntry;
	// copy over fields into new Fat16Entry, and set unimportant fields to 0
	strncpy((char*)(newEntry.filename), filename, 8);
	strncpy((char*)(newEntry.ext), ext, 3);
	memset(&(newEntry.attributes), 0, 1);
	memset(&(newEntry.reserved), 0, 10);
	memset(&(newEntry.modify_time), 0, sizeof(unsigned short));
	memset(&(newEntry.modify_date), 0, sizeof(unsigned short));
	newEntry.starting_cluster = startingCluster;
	newEntry.file_size = file_size;
	//printDirEntry(newEntry); // for testing
	return newEntry;
}

unsigned int findEmptySpotDir(FILE* fatFile, long currentLocation) {
	/* Find empty spot in current directory; used for placing new file with copyin */ 
	// NEED LOCATION OF CURRENT DIRECTORY
	//Fat16Entry* checker = new Fat16ENtry;
	char* checker = new char;
	int i = 0;
	// seek to current location (the location of the directory currently in)
	fseek(fatFile, currentLocation, SEEK_SET);
	// first read to initialize checker
	fread(checker, 2, 1, fatFile);
	while(*checker != 0x00) {
		// seek forward 1 entry -2 because of length 2 previos read 
		fseek(fatFile, (sizeof(Fat16Entry))-2, SEEK_CUR);
		// read two characters
		fread(checker, 2, 1, fatFile);
		i++;
	}
	// -2 is because fread will read 2 characters ahead of point in file
	unsigned int openLoc = ftell(fatFile)-2;
	return openLoc;
}

void writeNewDir(FILE* fatFile, unsigned int newDirLoc, Fat16Entry* newEntry) {
	/* write a new dir entry to the FAT volume */
	fseek(fatFile, newDirLoc, SEEK_SET);
	fwrite(newEntry, 1, sizeof(Fat16Entry), fatFile);
}

void writeToData(int starting_cluster, FILE* fatFile, Fat16BootSector* b, FILE* systemFile, unsigned int writeBuffSize, unsigned int systemIndex) {
	/* write chunk of data to volume of size "writeBuffSize" */	
	int cluster_size = b->sector_size * b->sectors_per_cluster;
	unsigned int dataLoc = getFileLocation(starting_cluster, fatFile, b);
	
	// create buffer of size passed in
	char writeBuff[writeBuffSize];
	
	// seek to spot in system to read data from and read into writeBuff
	fseek(systemFile, (systemIndex * cluster_size), SEEK_SET);
	fread(writeBuff, sizeof(char), writeBuffSize, systemFile);
	// seek to spot in data to write to and write data from writeBuff
	fseek(fatFile, dataLoc, SEEK_SET);
	fwrite(writeBuff, sizeof(char), writeBuffSize, fatFile);
}

void copyIn(char* newName, Fat16BootSector* b, FILE* fatFile, FILE* systemFile, long* currentLocation) {
	/* Handles bulk of the work for copying files from the local file system to the FAT Volume */
	// format filename and extension
	char* ext = getFilenameAndExt(&newName);
	char resizedFileName[8];
	makeIntoEight(newName, resizedFileName);
	
	// create variables for the first fat location and the cluster size
	int cluster_size = b->sector_size * b->sectors_per_cluster;
	unsigned int first_fat_location = b->reserved_sectors * b->sector_size;
	
	// create buffer to hold FAT table and read it in 
	unsigned short fat_table[b->fat_size_sectors * b->sector_size];
	fseek(fatFile, first_fat_location, SEEK_SET);
	fread(fat_table, (b->fat_size_sectors * b->sector_size), 1, fatFile);
	
	// get size of system file by seeking to end and noting where this is
	fseek(systemFile, 0L, SEEK_END);
	unsigned int systemFileSize = ftell(systemFile);

	// create new starting cluster location based on first open location in FAT table
	int newStartingCluster = findOpenFatLoc(fat_table);
	// create new directory entry and write it at the first empty location in the current directory
	Fat16Entry newFile = createFat16Entry(newName, ext, fatFile, systemFileSize, (unsigned short) newStartingCluster);
	unsigned int newDirLoc = findEmptySpotDir(fatFile, *currentLocation); // NEED TO CHANGE TO CORRECT PATH
	writeNewDir(fatFile, newDirLoc, &newFile);

	int i;
	int prevFatIndex;
	int fatUpdateIndex;
	
	for(i=0; i<((systemFileSize-1)/cluster_size); i++) {
		newStartingCluster = findOpenFatLoc(fat_table);
		// placeholder of 1 so that spot will not be seen as free before update
		fat_table[newStartingCluster] = 1;
		// advance through "linked list"; update previous to current and current to next
		prevFatIndex = fatUpdateIndex;
		fatUpdateIndex = newStartingCluster;
  	if(i>0) {
			// Create update previous index to "point" to new index 
			fat_table[prevFatIndex] = fatUpdateIndex;
		}
		// write the data of the file
		writeToData(newStartingCluster, fatFile, b, systemFile, cluster_size, i);
	}
	// update starting cluster to open location in FAT table
	newStartingCluster = findOpenFatLoc(fat_table);
	
	// advance through "linked list"
	prevFatIndex = fatUpdateIndex;
	fatUpdateIndex = newStartingCluster;

  if(systemFileSize > cluster_size) { 
		// if other FAT locations were used, seek to previously modified FAT location and write next (now current) location into it
		fat_table[prevFatIndex] = fatUpdateIndex;
		// write entire fat out
		fseek(fatFile, first_fat_location, SEEK_SET);
		fwrite(fat_table, (b->fat_size_sectors * b->sector_size), 1, fatFile);
	}

	unsigned int fatUpdateLoc = first_fat_location + newStartingCluster * sizeof(short);
	// signal that this is last cluster
	unsigned short end = 0xFFFF;
	fseek(fatFile, fatUpdateLoc, SEEK_SET);
	fwrite(&end, sizeof(short), 1, fatFile);

	// if remaining is exactly the size of a cluster, write it all
	if(systemFileSize%cluster_size == 0) {
		writeToData(newStartingCluster, fatFile, b, systemFile, cluster_size, i);
	}
	// otherwise only write remaining data, smaller than a cluster
	else {
		writeToData(newStartingCluster, fatFile, b, systemFile, (systemFileSize % cluster_size), i);
	}
}

////////////////////////////
/**   COMMAND HANDLERS   **/
////////////////////////////

void handleLs(char* input, long* currentLocation, Fat16Entry** currentDirectory, FILE* fatFile, Fat16BootSector* b, int rootDirLoc) {
	/* Function called when "ls" is entered; prints out applicable files in a given directory */
	// save starting location
	long locToRestore = *currentLocation;
	// ls current directory
	if(strncmp((input + 3), "", 3)==0) {
		*currentDirectory = readInDir(*currentLocation, fatFile);
		printDir(*currentDirectory);
	}
	else {
		// parse input using new tokens array
	  char** tokens = new char*[101 * sizeof(char*)];
	  char* token = strtok((input+3), "/");
	  int numTokens;
	  createTokenArray(token, tokens, &numTokens, "/");
		int i;
		
		for(i=0; i<numTokens; i++) {
			long dirLoc = findDir(tokens[i], fatFile, *currentDirectory, b);
			if(dirLoc==-1) {
				//cout << "invalid directory\n";
				break;
			}
			else {
				// change location (and directory)
				*currentLocation = dirLoc;
				*currentDirectory = readInDir(*currentLocation, fatFile);
			}
			if(i==numTokens-1) {
				*currentDirectory = readInDir(*currentLocation, fatFile);
				printDir(*currentDirectory);	
			}
		}
		delete[] tokens;
	}
	// restore original location
	*currentLocation = locToRestore;
}

void handleCd(char* input, long* currentLocation, Fat16Entry** currentDirectory, FILE* fatFile, Fat16BootSector* b, int rootDirLoc, stack<string> *cdNameStack, string* cdName) {
	/* Command called when user enters "cd" - changes current directory to one specified by a relative path */
	if(strncmp((input + 3), "", 3)==0) {
		*currentLocation = rootDirLoc;
	}
	else {
		// tokenize path for cd
	  char** tokens = new char*[101 * sizeof(char*)];
	  char* token = strtok((input+3), "/");
	  int numTokens;
	  createTokenArray(token, tokens, &numTokens, "/");
		int i;
		
		for(i=0; i<numTokens; i++) {
			// continually change location based on path entered
			long dirLoc = findDir(tokens[i], fatFile, *currentDirectory, b);
			// invalid input check
			if(dirLoc==-1) {
				cout << "invalid directory\n";
				break;
			}
			else {
				// do nothing if current director (.) entered
				if(strncmp(tokens[i], ".", 2)==0);
				// if we need to go up, pop off stack and change printed pathname
				else if(strncmp(tokens[i], "..", 3)==0) {
					cdNameStack->pop();
					// if going up, usually need to remove last / and after
					if(cdNameStack->size() > 1) {
						cdName->erase(cdName->rfind('/'));
					}
					// if going to root, keep last /
					else {
						cdName->erase(cdName->rfind('/')+1);	
					}
				}
				else {
					// traversing down a path means adding to printed directory
					cdNameStack->push((string)tokens[i]);
					if(cdNameStack->size() == 2) {
						*cdName += (string)tokens[i];
					}	
					else {
						*cdName += "/" + (string)tokens[i];
					}
				}
				// change directory location (general case)
				*currentLocation = dirLoc;
				*currentDirectory = readInDir(*currentLocation, fatFile);
			}
		}
		delete[] tokens;
	}
}

void handleCpout(char* input, long* currentLocation, Fat16Entry** currentDirectory, FILE* fatFile, Fat16BootSector* b) {
	/* Function called when user enters cpout; copies file from FAT volume to local file system */
	// The bulk of the work is done in the copyOut function
	*currentDirectory = readInDir(*currentLocation, fatFile);
	if(strncmp((input + 6), "", 3)==0) {
		cout << "must request file for copyout\n";
	}
	else {
		// parse the entered arguments
	  char** args = new char*[3 * sizeof(char*)];
	  char* arg1 = strtok(input, " ");
	  int numArgs;
	  createTokenArray(arg1, args, &numArgs, " ");
	 
	 	// new file
		FILE* systemFile = fopen(args[2], "wb");
		// give all permissions to everyone
		chmod(args[2], 0777);
		// process input by tokenizing pathname
		char** tokens = new char*[101 * sizeof(char*)];
	  char* token = strtok((input+6), "/");
	  int numTokens;
	  createTokenArray(token, tokens, &numTokens, "/");
		
		// save starting locaiton
		long locToRestore = *currentLocation;

		int i;
		for(i=0; i<numTokens; i++) {
			// For last token in path
			if(i==numTokens-1) {
				// get extension and isolate filename
				char* ext = getFilenameAndExt(&tokens[i]);
				// find index of file and copy it out	
				int fileIndex = findFileIndex(tokens[i], ext, fatFile, *currentDirectory, b);
				copyOut(fatFile, (*currentDirectory)[fileIndex], b, systemFile);
			}
			// change directory along path (does check for invalid directory
			else {
				long dirLoc = findDir(tokens[i], fatFile, *currentDirectory, b);
				if(dirLoc==-1) {
					cout << "invalid directory\n";
					break;
				}
				else {
					*currentLocation = dirLoc;
					*currentDirectory = readInDir(*currentLocation, fatFile);
				}
			}	
		}
		// cleanup - close system file, restore old location and directory, and garbage collect
		fclose(systemFile);
		*currentLocation = locToRestore;
		*currentDirectory = readInDir(*currentLocation, fatFile);
		delete[] tokens;
	}
}


void handleCpin(char* input, long* currentLocation, Fat16Entry** currentDirectory, FILE* fatFile, Fat16BootSector* b) {
	/* Function called when user enters cpin; copies file from local file system to FAT Volume */
	// The bulk of the work is done in the copyIn function
	if(strncmp((input + 6), "", 3)==0) {
		cout << "must request file for copyin\n";
	}
	else {
		// create tokens based on arguments
	  char** args = new char*[3 * sizeof(char*)];
	  char* arg1 = strtok(input, " ");
	  int numArgs;
	  createTokenArray(arg1, args, &numArgs, " ");
	 
	 	// open file for reading in binary mode
		FILE* systemFile = fopen(args[1], "r+b");
		if(systemFile == NULL) {
			cout << "Invalid system file!\n";
			exit(1);
		}
		// create token array based on path
		char** tokens = new char*[101 * sizeof(char*)];
	  char* token = strtok((args[2]), "/");
	  int numTokens;
	  createTokenArray(token, tokens, &numTokens, "/");
		int i;
		
		// save starting location
		long locToRestore = *currentLocation;

		for(i=0; i<numTokens; i++) {
			if(i==numTokens-1) {
				//cout << "copyout dirloc is: " << *currentLocation << endl;
				copyIn(tokens[i], b, fatFile, systemFile, currentLocation);
			}
			else {
				long dirLoc = findDir(tokens[i], fatFile, *currentDirectory, b);
				if(dirLoc==-1) {
					cout << "invalid directory\n";
					break;
				}
				else {
					*currentLocation = dirLoc;
					*currentDirectory = readInDir(*currentLocation, fatFile);
				}
			}	
		}
		// garbage collection, close file, and restore old directory and location
		delete[] args;
		delete[] tokens;
		fclose(systemFile);
		*currentLocation = locToRestore;
		*currentDirectory = readInDir(*currentLocation, fatFile);
	}
}


////////////////////////////
/** END COMMAND HANDLERS **/
////////////////////////////

int main ( int argc, char *argv[] ) {	
	// open up the file for reading and writing in binary mode
	FILE * fatFile = fopen(argv[1], "r+b");
	// create char buffer to store information from boot sector
	char bootBuff[1000 * sizeof(char)];
	// create a pointer to a boot sector object
	Fat16BootSector* b = new Fat16BootSector;
	// read from fatFile into buffer, and cast the data into boot sector pointer
	fread(bootBuff, sizeof(Fat16BootSector), 1, fatFile);
	b = (Fat16BootSector*) bootBuff;
	
	// Hold the location and size of the root directory	
	int rootDirLoc = computeRootDirLocation(b);
	int rootDirSize = b->root_dir_entries;
	
	// read in root directory by writing to buffer, casting to pointer to first entry in root
	char buff[512 * sizeof(Fat16Entry)];
	Fat16Entry* rootDir = new Fat16Entry[512];
	fseek(fatFile, rootDirLoc, SEEK_SET);
	fread(buff, 512 * sizeof(Fat16Entry), 1, fatFile);
	rootDir = (Fat16Entry*) buff;

	// create variable to hold current location and start at root
	long currentLocation = rootDirLoc;
	// create pointer to current directory
	Fat16Entry* currentDirectory = new Fat16Entry;
	currentDirectory = rootDir;
	// stack used for printing write path in prompt
	stack<string> cdNameStack;
	string cdName = "/";
	cdNameStack.push("/");

	while(1) {
		// input from user
		char* input = (char*)(malloc(102 * sizeof(char)));
    memset(input, 0, 102 * sizeof(char));
		// prompt with :<pathname>>
		cout << ":" << cdName << "> ";
		fgets(input, 101, stdin);
		char* endOfInput = input+strlen(input)-1;
		// remove newline char
		if(strncmp(endOfInput,"\n",1)==0) {
			*remove(input, endOfInput, '\n') = '\0';
		}
		
    // check for exit string
    if(strncmp("exit", input, 5)==0) {
			fclose(fatFile);
			exit(0);
		}

		// process different commands with "handle functions"
		if(strncmp("ls", input, 2)==0) {
			handleLs(input, &currentLocation, &currentDirectory, fatFile, b, rootDirLoc);
		}

		if(strncmp("cd", input, 2)==0) {
			handleCd(input, &currentLocation, &currentDirectory, fatFile, b, rootDirLoc, &cdNameStack, &cdName);
		}

		else if(strncmp("cpout", input, 5)==0) {
			handleCpout(input, &currentLocation, &currentDirectory, fatFile, b);
    }

		else if(strncmp("cpin", input, 4)==0) {
			handleCpin(input, &currentLocation, &currentDirectory, fatFile, b);
		}
	}
	// some garbage collection
	return 0;
}
