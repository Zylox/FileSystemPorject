#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>;
#include <VirtualBlockStorage.h>

#include "../include/S15Filesystem.h"

#define VBS_FILENAME "HARD_DRIVE"

#define INODE_NUM 256

/*
 * Gives accesses to the VBS for all functions inside this .c
 * only. The fs_mount function will provide VBS structure
 * for usage with the vbs API functions
 * */
static VBS_Type *virtualBlockStorage;


static Inode_t inodes[INODE_NUM];

static unsigned char firstOpenInodeIdx(){
	
	int i;
	//start at one because the first inode is the root always;
	for(i=1; i < INODE_NUM; i++){
		if(inodes[i].metadata.fileLinked == 0){
			return (unsigned char) i;
		}
	}
	
	return 0;
}

static char* getDirectorysFromPath(const char* absolutePath){
	
	char* afnCopy = strdup(absoluteFilename);
	char* tokens = strtok(afnCopy, "/");
	char* directoryPath;
	
	while (tokens) {
		tokens = strtok(NULL, " ");
		
	}
}

int fs_mount() {
	
	vbs_initialize(VBS_FILENAME);
	virtualBlockStorage = vbs_open(VBS_FILENAME);	

	return 0;
}

int fs_create_file(const char* absoluteFilename,FileType fileType) {

	//find first open inode;
	unsigned char openIdx = firstOpenInodeIdx();
	if(openIdx == 0){
		perror("no open inodes");
		return -1;
	}
	
	/*typedef struct {
		char fileName[64];
		FileMetadata_t metaData;
		unsigned short blockPointers[8];
	} Inode_t;*/
	
	Inode_t newInode;
	char* afnCopy = strdup(absoluteFilename);
	char* tokens = strtok(afnCopy, "/");
	
	while (token) {
		
		token = strtok(NULL, " ");
	}
	strncpy(newInode.filename, , 64);
	
	return 0;
}


int fs_get_directory (const char* absolutePath, Directory_t* directoryContents) {
	
	return 0;


}

int fs_seek_within_file (const char* absoluteFilename, unsigned int offset, unsigned int orgin) {
	
	return 0;
}

int fs_remove_file(const char* absoluteFilename) {

	return 0;

}

int fs_write_file(const char* absoluteFilename, void* dataToBeWritten, unsigned int numberOfBytes) {
	return 0;
}

int fs_read_file(const char* absoluteFilename, void* dataToBeRead, unsigned int numberOfBytes) {
	return 0;
}

/*
 * Doesn't truncate the file, just closes the filesystem access
 * to the vbs file used.
 * */
int fs_unmount() {
	return vbs_close(virtualBlockStorage);
}


