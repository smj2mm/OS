#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

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

void printDirEntry(Fat16Entry* r) {
  cout << "filename: " << r->filename << "; size: " << sizeof(r->filename) << "\n";
  cout << "ext: " << r->ext << "; size: " << sizeof(r->ext) << "\n";
  cout << "attributes: " << r->attributes << "; size: " << sizeof(r->attributes) << "\n";
  cout << "reserved: " << r->reserved << "; size: " << sizeof(r->reserved) << "\n";
  cout << "modify_time: " << r->modify_time << "; size: " << sizeof(r->modify_time) << "\n";
  cout << "modify_date: " << r->modify_date << "; size: " << sizeof(r->modify_date) << "\n";
  cout << "starting_cluster: " << r->starting_cluster << "; size: " << sizeof(r->starting_cluster) << "\n";
  cout << "file_size: " << r->file_size << "; size: " << sizeof(r->file_size) << "\n";	
}

void printDir(Fat16Entry** dir, int dirSize) {
	int i;
	for(i=0; i<dirSize; i++) {
		//cout << i << "\n";
		if(strncmp((char*)(dir[i]->filename), "", 4) == 0) {
			break;
		}
		printDirEntry(dir[i]);
	}
}

void createDir(int rLoc, FILE* fatFile, int dirSize, Fat16Entry** directory) {
	//Fat16Entry** directory = new Fat16Entry*[dirSize * sizeof(char)];
	
	char rootBuff[1000 * sizeof(char)];
	//Fat16Entry* r = new Fat16Entry;
	fseek(fatFile, rLoc, SEEK_SET);
	fread(rootBuff, sizeof(Fat16Entry), 1, fatFile);
	//r = (Fat16Entry*) rootBuff;
	directory[0] = (Fat16Entry*) rootBuff;

	//*(directory[0]) = r;
	printDirEntry(directory[0]);
	int i = 1;

	while(1) { 
		cout << "---- DIR ----\n";
		
		char buff[1000 * sizeof(char)];
		//Fat16Entry* entry = new Fat16Entry;
		fseek(fatFile, 0, SEEK_CUR);
		fread(buff, sizeof(Fat16Entry), 1, fatFile);
		//entry = (Fat16Entry*) buff;
		directory[i] = (Fat16Entry*) buff;

		if(strncmp((char*)(directory[i]->filename), "", 4) ==0) {
			break;
		}
		//directory[0][i] = entry;
		cout << i << ": ";
		//printDirEntry(entry);
		cout << "READ BACK\n";
		printDirEntry(directory[i]);
		i++;
		cout << "-------- \n\n";
		//sleep(1);
	}
}

int computeRootDirLocation(Fat16BootSector* b) {
	return (b->reserved_sectors + (b->fat_size_sectors * b->number_of_fats)) * b->sector_size;
}

int computeDataLocation(Fat16BootSector* b) {
	return (b->reserved_sectors + (b->fat_size_sectors * b->number_of_fats) + sizeof(Fat16Entry)) * b->sector_size;
}

int main ( int argc, char *argv[] ) {
	
	FILE * fatFile = fopen(argv[1], "rb");
	unsigned char ch;
	char bootBuff[1000 * sizeof(char)];
	Fat16BootSector* b = new Fat16BootSector;
	//fseek(fatFile, 0, SEEK_SET);
	fread(bootBuff, sizeof(Fat16BootSector), 1, fatFile);
	b = (Fat16BootSector*) bootBuff;
	
	printBootSector(b);
	//cout << "root offset: " << computeRootDirLocation(b);
	int rootDirLoc = computeRootDirLocation(b);
	int rootDirSize = b->root_dir_entries;
	
	cout << "Fat16Entry size: " << sizeof(Fat16Entry) << "\n";
	
	Fat16Entry** rootDirectory = new Fat16Entry*[rootDirSize * sizeof(char)];
	int i;
	for(i=0; i<rootDirSize; i++) {
		rootDirectory[i] = new Fat16Entry;		
	}
	createDir(rootDirLoc, fatFile, rootDirSize, rootDirectory);
	printDir(rootDirectory, rootDirSize);
	
	int dataDirLoc = computeDataLocation(b);
	int dirSize = b->root_dir_entries; // WRONG
	Fat16Entry** dataStart = new Fat16Entry*[rootDirSize * sizeof(char)];
	createDir(dataDirLoc, fatFile, dirSize, dataStart);

	//printBootSector(b);
	//printDirEntry(r);

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
		
		cout << "input: " << input << "\n";
    // check for exit string
    if(strncmp("exit\n", input, 5)==0) {
			exit(0);
		}
		cout << computeDataLocation(b) << "\n";
		/* NEW CODE */
		//fseek
		//fread

				
		//cout << noPointer.sector_size << "\n";
		//cout << b -> root_dir_entries << "\n";
	}
	return 0;
}

// MBR - Boot Sector is part of MBR
