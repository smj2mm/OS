#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

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
  unsigned long start_sector;
  unsigned long length_sectors;
} __attribute((packed)) PartitionTable;

typedef struct {
  unsigned char filename[8];
  unsigned char ext[3];
  unsigned char attributes;
  unsigned char reserved[10];
  unsigned short modify_time;
  unsigned short modify_date;
  unsigned short starting_cluster;
  //unsigned long file_size;
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

void createTokenArray(char* token, char** tokens, int* numTokens) {
  int i = 0;
  while(token != NULL) {
		// create space, add token
    tokens[i] = (char*) malloc(sizeof(token));
    tokens[i] = token;
    token = strtok(NULL, "/");
		if(token!=NULL) {
			*remove(token, token+strlen(token), '\n') = '\0';
		}
		i++;
    // update number of tokens
		*numTokens=i;
  }
}

bool checkEntry(Fat16Entry f) {
  /*
	0x01 Read-only
  0x02 Hidden
  0x04 System
  0x08 Volume label
  0x10 Subdirectory
  0x20 Archive
	*/
	
	if((f.attributes & HIDDEN) == HIDDEN) {
		//cout << "hidden" << "\n";
		return false;
	}
	if((f.attributes & SYSTEM) == SYSTEM) {
		//cout << "system" << "\n";
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

void printDir(Fat16Entry* dir, int dirSize) {
	int i;
	for(i=0; i<dirSize; i++) {
		//printDirEntry(dir[i]);
		if(checkEntry(dir[i])) {
		  printf ("%.8s\n", dir[i].filename);
			//cout << dir[i].filename << "\n";
		}
	}
}

void getDirSize(Fat16Entry* dir, int* size) {
	int i = 0;
	while(1) {
		//cout << i << "\n";
		//cout << dir[i].filename;
		if(strncmp((char*)(dir[i].filename), "", 4) ==0) {
			*size = i;
			break;
		}
		//sleep(1);
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
	
	getDirSize(dir, &dirNumEntries);
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

int getDirLocation(Fat16Entry f, FILE* fatFile, Fat16BootSector* b) {
	if(f.starting_cluster == 0) {
		// special case for root
		return computeRootDirLocation(b);
	}
	int blocksToData = b->reserved_sectors + (b->fat_size_sectors * b->number_of_fats) + sizeof(Fat16Entry);
	return (((f.starting_cluster-2)*(b->sectors_per_cluster)) + blocksToData) * b->sector_size;
	//int blocksToRootDir = b->reserved_sectors + (b->fat_size_sectors * b->number_of_fats);
	//return (((f.starting_cluster-2)*(b->sectors_per_cluster)) + blocksToRootDir) * b->sector_size;
}

int findDir(char* name, FILE* fatFile, Fat16Entry* dir, int dirSize, Fat16BootSector* b) {
	// do ls and compare filenames with name
	// if the name matches, check if it's a directory
	int i;
	for(i=0; i<dirSize; i++) {
		//printDirEntry(dir[i]);
		if(strncmp((char*)dir[i].filename, name, strlen(name)-1)==0) {
			if(isDirectory(dir[i], fatFile)) {
				//return dir[i].starting_cluster;
				return getDirLocation(dir[i], fatFile, b);
			}
		}
	}
	return -1;
}

int main ( int argc, char *argv[] ) {
	
	FILE * fatFile = fopen(argv[1], "rb");
	unsigned char ch;
	char bootBuff[1000 * sizeof(char)];
	Fat16BootSector* b = new Fat16BootSector;
	//fseek(fatFile, 0, SEEK_SET);
	fread(bootBuff, sizeof(Fat16BootSector), 1, fatFile);
	b = (Fat16BootSector*) bootBuff;
	
	//printBootSector(b);
	
	int rootDirLoc = computeRootDirLocation(b);
	int rootDirSize = b->root_dir_entries;
	
	//cout << "rootDirLoc: " << rootDirLoc << "\n";
	
	char buff[512 * sizeof(Fat16Entry)];
	Fat16Entry* rootDir = new Fat16Entry[512];
	fseek(fatFile, rootDirLoc, SEEK_SET);
	fread(buff, 512 * sizeof(Fat16Entry), 1, fatFile);
	rootDir = (Fat16Entry*) buff;
	int rootDirNumEntries;

	getDirSize(rootDir, &rootDirNumEntries);

	//printDir(rootDir, rootDirNumEntries);


	int dataDirLoc = computeDataLocation(b);
	//int dirSize = b->root_dir_entries; // WRONG
	
	//cout << dataDirLoc << "\n\n";
	Fat16Entry* dataStart = new Fat16Entry;
	
	dataStart = readInDir(dataDirLoc, fatFile);
	int currentLocation = rootDirLoc;
	Fat16Entry* currentDirectory = new Fat16Entry;
	
	currentDirectory = rootDir;
	int dirSize = rootDirNumEntries;

	//cout << "Here's where parent is: " << getDirLocation(dataStart[1], fatFile, b) << "\n";

	while(1) {
		/*
		create character buffer with plenty of room
		seak and read in sector 0 in buffer 
			-- read(buff, 512)
		Fat16BootSector* b;
		b = (Fat16BootSector*) buff;
		b -> numFats... etc.
		*/
  	
		char* input = (char*)(malloc(102 * sizeof(char)));
    memset(input, 0, 102 * sizeof(char));
    fgets(input, 101, stdin);
		
		//cout << "input: " << input << "\n";
    // check for exit string
    if(strncmp("exit\n", input, 5)==0) {
			exit(0);
		}
		
		if(strncmp("ls", input, 2)==0) {
			currentDirectory = readInDir(currentLocation, fatFile);
			getDirSize(currentDirectory, &dirSize);
			printDir(currentDirectory, dirSize);
		}

		if(strncmp("cd", input, 2)==0) {
			if(strncmp((input + 3), "", 3)==0) {
				currentLocation = rootDirLoc;
				//cout << "current location: " << currentLocation << "\n";
			}
			else {
			  char** tokens = new char*[101 * sizeof(char*)];
			  char* token = strtok((input+3), "/");
			  int numTokens;
			  createTokenArray(token, tokens, &numTokens);
				// First just cd into one
				int dirLoc = findDir(input+3, fatFile, currentDirectory, dirSize, b);
				if(dirLoc==-1) {
					cout << "invalid directory\n";
				}
				else {
					cout << "location: " << dirLoc << "\n";
					currentLocation = dirLoc;
					currentDirectory = readInDir(currentLocation, fatFile);
				}
			}
		}
		/*
		if(strncmp("cd ", input, 3)==0) {

		}*/
		
	}
	return 0;
}

// MBR - Boot Sector is part of MBR
