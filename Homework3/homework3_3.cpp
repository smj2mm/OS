#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <string>
#include <cstring>
#include <algorithm> 

#include <stack>  // std::stack
#include <vector>
 
#define	READ_ONLY 		0x01
#define HIDDEN 				0x02
#define SYSTEM 				0x04
#define VOLUME_LABEL 	0x08
#define SUBDIRECTORY 	0x10
#define ARCHIVE 			0x20

using namespace std;

typedef struct {
  unsigned char first_byte;
  unsigned char start_chs[3];
  unsigned char partition_type;
  unsigned char end_chs[3];
  unsigned int start_sector;
  unsigned int length_sectors;
} __attribute((packed)) PartitionTable;

typedef struct {
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
  unsigned char jmp[3];
  char oem[8];
  unsigned short sector_size; // 2 bytes
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
  int i = 0;
  while(token != NULL) {
		// create space, add token
    tokens[i] = (char*) malloc(sizeof(token));
    tokens[i] = token;
    //token = strtok(NULL, "/");
		token = strtok(NULL, delimiter);
		if(token!=NULL) {
			char* endOfToken = token+strlen(token)-1;
			if(strncmp(endOfToken,"\n",1)==0) {
				*remove(token, endOfToken, '\n') = '\0';
				//cout << "token with newline removed: "<< token << "\n";
			}
		}
		i++;
    // update number of tokens
		*numTokens=i;
  }
}

bool checkEntry(Fat16Entry f) {
	if((f.attributes & HIDDEN) == HIDDEN) {
		//cout << "hidden" << "\n";
		return false;
	}
	if((f.attributes & SYSTEM) == SYSTEM) {
		//cout << "system" << "\n";
		return false;
	}
	if((f.attributes & VOLUME_LABEL) == VOLUME_LABEL) {
		return false;
	}
	return true;
}

void printDirEntry(Fat16Entry r) {
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
	int i=0;
	while(dir[i].filename[0] != 0x00) {
		//printDirEntry(dir[i]);
		if(checkEntry(dir[i])) {
			if((dir[i].attributes & SUBDIRECTORY) == SUBDIRECTORY) {
				printf ("%.8s\n", dir[i].filename);
			}
			else {
				printf ("%.8s.%.3s\n", dir[i].filename, dir[i].ext);
				//cout << dir[i].filename << "\n";
			}
		}
		i++;
	}
}

Fat16Entry* readInDir(int dirLoc, FILE* fatFile) {
	char buff[1000 * sizeof(Fat16Entry)];
	Fat16Entry* dir = new Fat16Entry[1000];
	fseek(fatFile, dirLoc, SEEK_SET);
	fread(buff, 1000 * sizeof(Fat16Entry), 1, fatFile);
	dir = (Fat16Entry*) buff;
	int dirNumEntries;
	
	//getDirSize(dir, &dirNumEntries);
	//printDir(dir, dirNumEntries);
	return dir;
}

int computeRootDirLocation(Fat16BootSector* b) {
	return (b->reserved_sectors + (b->fat_size_sectors * b->number_of_fats)) * b->sector_size;
}

int computeDataLocation(Fat16BootSector* b) {
	return (b->reserved_sectors + (b->fat_size_sectors * b->number_of_fats) + sizeof(Fat16Entry)) * b->sector_size;
}

bool isDirectory(Fat16Entry f, FILE* fatFile) {
	if((f.attributes & SUBDIRECTORY) == SUBDIRECTORY) {
		//cout << "subdirectory" << "\n";
		return true;
	}
	return false;
}

unsigned int getFileLocation(int starting_cluster, FILE* fatFile, Fat16BootSector* b) {
	// Given starting cluster, finds location in data region
	if(starting_cluster == 0) {
		// special case for root
		return computeRootDirLocation(b);
	}
	int blocksToData = b->reserved_sectors + (b->fat_size_sectors * b->number_of_fats) + sizeof(Fat16Entry);
	return (((starting_cluster-2)*(b->sectors_per_cluster)) + blocksToData) * b->sector_size;
	//int blocksToRootDir = b->reserved_sectors + (b->fat_size_sectors * b->number_of_fats);
	//return (((f.starting_cluster-2)*(b->sectors_per_cluster)) + blocksToRootDir) * b->sector_size;
}

unsigned short getTableValue(int sector_size, int active_cluster, int first_fat_sector, FILE* fatFile) {
	unsigned char FAT_table[sector_size * sizeof(Fat16Entry)];
  memset(FAT_table, 0, sector_size * sizeof(char));

	unsigned int fat_offset = active_cluster * 2;
	unsigned int fat_sector = first_fat_sector + (fat_offset / sector_size);
	unsigned int ent_offset = fat_offset % sector_size;

	//at this point you need to read from sector "fat_sector" on the disk into "FAT_table".
	fseek(fatFile, fat_sector * sector_size, SEEK_SET);
	fread(FAT_table, sector_size, 1, fatFile);

	unsigned short table_value = *(unsigned short*)&FAT_table[ent_offset];
	return table_value; 
	//the variable "table_value" now has the information you need about the next cluster in the chain.
}

int findDir(char* name, FILE* fatFile, Fat16Entry* dir, Fat16BootSector* b) {
	// do ls and compare filenames with name
	// if the name matches, check if it's a directory
	int i,j; char* currFilename = new char[8];
	//switch to while loop
	i=0;
	while(dir[i].filename[0] != 0x00) {
		j=0;
		while(j<8) {
			// Kill whitespace
			if(dir[i].filename[j] == ' ') {
				break;
			}
			currFilename[j] = dir[i].filename[j];
			j++;
		}
		//cout << currFilename << "\n" << name << "\n ---- \n";
		//cout << "length: " << strlen(name) << "\n";
		if(strncmp(currFilename, name, strlen(name))==0) {
		//if(strcmp(currFilename, name)==0) {	
			if(isDirectory(dir[i], fatFile)) {
				//cout << "FOUND THIS: " << dir[i].filename << "\n";
				//if(realName != NULL) {
					//printDirEntry(dir[i]);
					//*realName = string(dir[i].filename, std::find(dir[i].filename, dir[i].filename+8, '\0'));
				//}
				return getFileLocation(dir[i].starting_cluster, fatFile, b);
			}
		}
		i++;
	}
	return -1;
}

int findFileIndex(char* name, char* ext, FILE* fatFile, Fat16Entry* dir, Fat16BootSector* b) {	
	int i,j; char* currFilename = new char[8];
	i=0;
	while(dir[i].filename[0] != 0x00)	{
		j=0;	
		while(j<8) {
			// Kill whitespace
			if(dir[i].filename[j] == ' ') {
				break;
			}
			currFilename[j] = dir[i].filename[j];
			j++;
		}
		//cout << currFilename << "\n" << name << "\n ---- \n";

		if(strncmp(currFilename, name, strlen(name)-1)==0) {
			//cout << "made it part way\n";
			if(strncmp((char*)dir[i].ext, ext, 3)==0) {
				//cout << "FOUND THIS: " << dir[i].filename << "\n";
				//cout << "first fat sector location is: " << b->reserved_sectors * b->sector_size << "\n";
				//cout << "getTableValue: " << getTableValue(b->sector_size, dir[i].starting_cluster, b->reserved_sectors, fatFile) << "\n";
				//return getFileLocation(dir[i], fatFile, b);
				//return dir[i];
				return i;
			}
		}
		i++;
	}
	return -1;
}

char* getFilenameAndExt(char **filename) {
  char *lastDot = strrchr(*filename, '.');
  //if(!dot || dot == filename) return "";
  if(lastDot != NULL)
		*lastDot = '\0';
	//if(!lastDot || lastDot == *filename) return "";
	return lastDot + 1;
}

void traceLinkedList(FILE* fatFile, Fat16Entry file, Fat16BootSector* b, int systemFile) {
	unsigned short nextFatLocation = getTableValue(b->sector_size, file.starting_cluster, b->reserved_sectors, fatFile);
	cout << "nextFatLocation: " << nextFatLocation << "\n";
	while(nextFatLocation < 0xFFF8) {
		nextFatLocation = getTableValue(b->sector_size, nextFatLocation, b->reserved_sectors, fatFile);
		cout << "nextFatLocation: " << nextFatLocation << "\n";
	}
}

void copyOut(FILE* fatFile, Fat16Entry file, Fat16BootSector* b, FILE* systemFile) {
	unsigned int locationInBytes;
	int cluster_size = b->sector_size * b->sectors_per_cluster;
	cout << "File size: " << file.file_size << "\n";
	if(file.file_size < cluster_size) {
		//char* readBuff2[file.file_size];
		cout << "small file. file size = " << file.file_size << "\n";
		char* readBuff[file.file_size];
		
		locationInBytes = getFileLocation(file.starting_cluster, fatFile, b);
		cout << "locationInBytes: " << locationInBytes << "\n";
		fseek(fatFile, locationInBytes, SEEK_SET);
		fread(readBuff, 1, file.file_size, fatFile);
		fwrite(readBuff, 1, file.file_size, systemFile);
	}
	else {
		//int cluster_size = b->sector_size * b->sectors_per_cluster;
		cout << "large file. cluster size: " << cluster_size << "\n";
		cout << "occurances of cluster size: " << file.file_size / cluster_size << "\n";
		cout << "    remaining: " << file.file_size % cluster_size << "\n";
		
		char* readBuff1[cluster_size];
		char* readBuff2[file.file_size % cluster_size];
		unsigned short nextStartingCluster = file.starting_cluster;
		
		while(nextStartingCluster < 0xFFF8) {
			locationInBytes = getFileLocation(nextStartingCluster, fatFile, b);
			nextStartingCluster = getTableValue(b->sector_size, nextStartingCluster, b->reserved_sectors, fatFile);	
			if(nextStartingCluster < 0xFFF8) {
				cout << "locationInBytes: " << locationInBytes << "\n";
				fseek(fatFile, locationInBytes, SEEK_SET);
				fread(readBuff1, 1, cluster_size, fatFile);
				fwrite(readBuff1, 1, cluster_size, systemFile);
			}
			else {
				cout << "last locationInBytes: " << locationInBytes << "\n";
				fseek(fatFile, locationInBytes, SEEK_SET);
				fread(readBuff2, 1, cluster_size, fatFile);
				fwrite(readBuff2, 1, cluster_size, systemFile);
			}
		}
	}
}

unsigned int findOpenFatLoc(unsigned short* fat_table) {
	int j=0;
	while(fat_table[j] != 0) {
		//cout << "-- " << fat_table[i] << "\n";
		j++;
	}
	return j;
}

Fat16Entry createFat16Entry(char* filename, char* ext, FILE* fatFile, unsigned int file_size, unsigned short startingCluster) {
  // unsigned char filename[8];
  // unsigned char ext[3];
  // unsigned char attributes;
  // unsigned char reserved[10];
  // unsigned short modify_time;
  // unsigned short modify_date;
  // unsigned short starting_cluster;
	// int32_t file_size; 
	Fat16Entry newEntry;
	cout << "filename: " << filename << "\n";
	strncpy((char*)(newEntry.filename), filename, 8);
	cout << "ext: " << ext << "\n";
	strncpy((char*)(newEntry.ext), ext, 3);
	memset(&(newEntry.attributes), 0, 1);
	memset(&(newEntry.reserved), 0, 10);
	memset(&(newEntry.modify_time), 0, sizeof(unsigned short));
	memset(&(newEntry.modify_date), 0, sizeof(unsigned short));
	fseek(systemFile, 0, SEEK_END);
	newEntry.starting_cluster = 65535;
	newEntry.file_size = ftell(systemFile);
	cout << "THIS FILESIZE IS: " << newEntry.file_size << "\n";
	printDirEntry(newEntry);
	return newEntry;
}

unsigned int findEmptySpotDir(FILE* fatFile, unsigned int currentLocation) {
	// NEED LOCATION OF CURRENT DIRECTORY
	//Fat16Entry* checker = new Fat16ENtry;
	char* checker = new char;
	int i = 0;
	fseek(fatFile, currentLocation, SEEK_SET);
	fread(checker, 2, 1, fatFile);
	while(*checker != 0x00) {
		fseek(fatFile, (sizeof(Fat16Entry))-2, SEEK_CUR);
		fread(checker, 2, 1, fatFile);
		i++;
	}
	//fwrite(*i, sizeof(unsigned short), 1, fatFile);
	unsigned int openLoc = ftell(fatFile)-2;
	return openLoc;
	//cout << "location: " << openLoc << endl;
	//fwrite(newEntry, 1, sizeof(Fat16Entry), fatFile);
}


void writeToFileAndData(unsigned short fat_table[], int firstFatLoc, FILE* fatFile, Fat16BootSector* b, unsigned int newDirLoc, Fat16Entry* newEntry, FILE* systemFile) {
	int prevFatTableLoc = firstFatLoc;
	unsigned int dataLoc = getFileLocation(prevFatTableLoc, fatFile, b);
	cout << "new data location: " << dataLoc << endl;
	cout << "new dir location: " << newDirLoc << endl;
	cout << "filesize: " << newEntry->file_size << endl;
	cout << "next cluster: " << newEntry->starting_cluster << "\n\n";

	fseek(fatFile, newDirLoc, SEEK_SET);
	fwrite(newEntry, 1, sizeof(Fat16Entry), fatFile);
	
	char writeBuff[newEntry->file_size];
	fseek(fatFile, dataLoc, SEEK_SET);
	fseek(systemFile, 0, SEEK_SET);
	fread(writeBuff, sizeof(char), (newEntry->file_size), systemFile);
	/*
	int i;
	for(i=0; i<newEntry->file_size; i++)
		writeBuff[i] = 'a';
	*/
	fwrite(writeBuff, sizeof(char), (newEntry->file_size), fatFile);
}

////////////////////////////
/**   COMMAND HANDLERS   **/
////////////////////////////

void handleLs(char* input, int* currentLocation, Fat16Entry** currentDirectory, FILE* fatFile, Fat16BootSector* b, int rootDirLoc) {
	int locToRestore = *currentLocation;
	if(strncmp((input + 3), "", 3)==0) {
		*currentDirectory = readInDir(*currentLocation, fatFile);
		printDir(*currentDirectory);
	}
	else {
	  char** tokens = new char*[101 * sizeof(char*)];
	  char* token = strtok((input+3), "/");
	  int numTokens;
	  createTokenArray(token, tokens, &numTokens, "/");
		int i;
		
		for(i=0; i<numTokens; i++) {
			cout << "attempting token: " << tokens[i] << "\n";
			int dirLoc = findDir(tokens[i], fatFile, *currentDirectory, b);
			if(dirLoc==-1) {
				cout << "invalid directory\n";
				break;
			}
			else {
				//cout << "i: " << i << "\n";
				//cout << "location: " << dirLoc << "\n";
				*currentLocation = dirLoc;
				*currentDirectory = readInDir(*currentLocation, fatFile);
			}
			if(i==numTokens-1) {
				*currentDirectory = readInDir(*currentLocation, fatFile);
				printDir(*currentDirectory);	
			}
		}
	}
	*currentLocation = locToRestore;
}

void handleCd(char* input, int* currentLocation, Fat16Entry** currentDirectory, FILE* fatFile, Fat16BootSector* b, int rootDirLoc, stack<string> *cdNameStack, string* cdName) {
	if(strncmp((input + 3), "", 3)==0) {
		*currentLocation = rootDirLoc;
		//cout << "current location: " << currentLocation << "\n";
	}
	else {
	  char** tokens = new char*[101 * sizeof(char*)];
	  char* token = strtok((input+3), "/");
	  int numTokens;
	  createTokenArray(token, tokens, &numTokens, "/");
		int i;
		
		for(i=0; i<numTokens; i++) {
			int dirLoc = findDir(tokens[i], fatFile, *currentDirectory, b);
			//cout << "tokens[" << i << "] = " << tokens[i] << "\n";
			if(dirLoc==-1) {
				cout << "invalid directory\n";
				break;
			}
			else {
				//cout << "i: " << i << "\n";
				//cout << "location: " << dirLoc << "\n";
				//cout << "current directory name: " << cdName << endl;
				if(strncmp(tokens[i], ".", 2)==0);
				else if(strncmp(tokens[i], "..", 3)==0) {
					cdNameStack->pop();
					if(cdNameStack->size() > 1) {
						cdName->erase(cdName->rfind('/'));
					}
					else {
						cdName->erase(cdName->rfind('/')+1);	
					}
				}
				else {
					cdNameStack->push((string)tokens[i]);
					if(cdNameStack->size() == 2) {
						//cout << "stack has only one\n";
						*cdName += (string)tokens[i];
					}	
					else {
						*cdName += "/" + (string)tokens[i];
					}
				}
				*currentLocation = dirLoc;
				*currentDirectory = readInDir(*currentLocation, fatFile);
			}
		}
	}
}

void handleCpout(char* input, int* currentLocation, Fat16Entry** currentDirectory, FILE* fatFile, Fat16BootSector* b) {
	*currentDirectory = readInDir(*currentLocation, fatFile);
	if(strncmp((input + 6), "", 3)==0) {
		cout << "must request file for copyout\n";
		//cout << "current location: " << currentLocation << "\n";
	}
	else {
	  char** args = new char*[3 * sizeof(char*)];
	  char* arg1 = strtok(input, " ");
	  int numArgs;
	  createTokenArray(arg1, args, &numArgs, " ");
	 
		//int systemFile = open(args[2], O_WRONLY | O_CREAT | O_TRUNC, 00664);
		FILE* systemFile = fopen(args[2], "wb");

		char** tokens = new char*[101 * sizeof(char*)];
	  char* token = strtok((input+6), "/");
	  int numTokens;
	  createTokenArray(token, tokens, &numTokens, "/");
		
		int locToRestore = *currentLocation;
		Fat16Entry* dirToRestore = new Fat16Entry;
		dirToRestore = *currentDirectory;

		int i;
		for(i=0; i<numTokens; i++) {
			if(i==numTokens-1) {
				char* ext = getFilenameAndExt(&tokens[i]);
				//createFat16Entry(tokens[i], ext, fatFile, systemFile);
					
				int fileIndex = findFileIndex(tokens[i], ext, fatFile, *currentDirectory, b);
				copyOut(fatFile, (*currentDirectory)[fileIndex], b, systemFile);
				//int fileLoc = traceLinkedList(tokens[i], fatFile, currentDirectory, b);
				//cout << "file loc is: " << fileLoc << "\n";
			}
			else {
				int dirLoc = findDir(tokens[i], fatFile, *currentDirectory, b);
				//cout << "tokens[" << i << "] = " << tokens[i] << "\n";
				if(dirLoc==-1) {
					cout << "invalid directory\n";
					break;
				}
				else {
					//cout << "i: " << i << "\n";
					//cout << "location: " << dirLoc << "\n";
					*currentLocation = dirLoc;
					*currentDirectory = readInDir(*currentLocation, fatFile);
				}
			}	
		}
		fclose(systemFile);
		*currentLocation = locToRestore;
		*currentDirectory = dirToRestore;
	}
}


void handleCpin(char* input, int* currentLocation, Fat16Entry** currentDirectory, FILE* fatFile, Fat16BootSector* b) {
	if(strncmp((input + 6), "", 3)==0) {
		cout << "must request file for copyout\n";
		//cout << "current location: " << currentLocation << "\n";
	}
	else {
	  char** args = new char*[3 * sizeof(char*)];
	  char* arg1 = strtok(input, " ");
	  int numArgs;
	  createTokenArray(arg1, args, &numArgs, " ");
	 
		//int systemFile = open(args[2], O_WRONLY | O_CREAT | O_TRUNC, 00664);
		FILE* systemFile = fopen(args[1], "r+b");
		if(systemFile == NULL)
			cout << "AHHH!! ERROR!\n";

		char** tokens = new char*[101 * sizeof(char*)];
	  char* token = strtok((args[2]), "/");
	  int numTokens;
	  createTokenArray(token, tokens, &numTokens, "/");
		int i;
		
		int locToRestore = *currentLocation;
		Fat16Entry* dirToRestore = new Fat16Entry;
		dirToRestore = *currentDirectory;

		for(i=0; i<numTokens; i++) {
			if(i==numTokens-1) {
				char* ext = getFilenameAndExt(&tokens[i]);
				
				unsigned int first_fat_location = b->reserved_sectors * b->sector_size;
				
				unsigned short fat_table[b->fat_size_sectors * b->sector_size];
				fseek(fatFile, first_fat_location, SEEK_SET);
				fread(fat_table, (b->fat_size_sectors * b->sector_size), 1, fatFile);
				
				fseek(systemFile, 0L, SEEK_END);
				unsigned int systemFileSize = ftell(systemFile);
				
				int newStartingCluster = findOpenFatLoc(fat_table);
				Fat16Entry newFile = createFat16Entry(tokens[i], ext, fatFile, systemFileSize, (unsigned short) newStartingCluster);
				
				int fatUpdateLoc = first_fat_location + newStartingCluster * sizeof(short);
				cout << "update location: " << fatUpdateLoc << endl;

				fseek(fatFile, fatUpdateLoc, SEEK_SET);
				unsigned short end = 0xFFFF;
				fwrite(&end, 1, 1, fatFile);
				
				// NEED TO CHECK FOR CASE WHERE FILE ALREADY EXISTS
				unsigned int newDirLoc = findEmptySpotDir(fatFile, *currentLocation); // NEED TO CHANGE TO CORRECT PATH
				writeToFileAndData(fat_table, newStartingCluster, fatFile, b, newDirLoc, &newFile, systemFile);
				//copyIn(fatFile, currentDirectory[fileIndex], b, systemFile);
			}
			else {
				int dirLoc = findDir(tokens[i], fatFile, *currentDirectory, b);
				//cout << "tokens[" << i << "] = " << tokens[i] << "\n";
				if(dirLoc==-1) {
					cout << "invalid directory\n";
					break;
				}
				else {
					//cout << "i: " << i << "\n";
					//cout << "location: " << dirLoc << "\n";
					*currentLocation = dirLoc;
					*currentDirectory = readInDir(*currentLocation, fatFile);
				}
			}	
		}
		fclose(systemFile);
		*currentLocation = locToRestore;
		*currentDirectory = dirToRestore;
	}
}


////////////////////////////
/** END COMMAND HANDLERS **/
////////////////////////////

int main ( int argc, char *argv[] ) {	
	FILE * fatFile = fopen(argv[1], "r+b");
	unsigned char ch;
	char bootBuff[1000 * sizeof(char)];
	Fat16BootSector* b = new Fat16BootSector;
	fread(bootBuff, sizeof(Fat16BootSector), 1, fatFile);
	b = (Fat16BootSector*) bootBuff;
	
	//printBootSector(b);
	
	int rootDirLoc = computeRootDirLocation(b);
	int rootDirSize = b->root_dir_entries;
	
	char buff[512 * sizeof(Fat16Entry)];
	Fat16Entry* rootDir = new Fat16Entry[512];
	fseek(fatFile, rootDirLoc, SEEK_SET);
	fread(buff, 512 * sizeof(Fat16Entry), 1, fatFile);
	rootDir = (Fat16Entry*) buff;
	int rootDirNumEntries;

	//printDir(rootDir);

	int dataDirLoc = computeDataLocation(b);
	
	//cout << dataDirLoc << "\n\n";
	Fat16Entry* dataStart = new Fat16Entry;
	
	dataStart = readInDir(dataDirLoc, fatFile);
	int currentLocation = rootDirLoc;
	Fat16Entry* currentDirectory = new Fat16Entry;
	
	currentDirectory = rootDir;
	int dirSize = rootDirNumEntries;
	stack<string> cdNameStack;
	string cdName = "/";
	cdNameStack.push("/");

	//cout << "Here's where parent is: " << getDirLocation(dataStart[1], fatFile, b) << "\n";

	while(1) {
		char* input = (char*)(malloc(102 * sizeof(char)));
    memset(input, 0, 102 * sizeof(char));
    cout << ":" << cdName << "> ";
		fgets(input, 101, stdin);
		char* endOfInput = input+strlen(input)-1;
		if(strncmp(endOfInput,"\n",1)==0) {
			*remove(input, endOfInput, '\n') = '\0';
		}
		
    // check for exit string
    if(strncmp("exit", input, 5)==0) {
			exit(0);
		}
		
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
	return 0;
}
