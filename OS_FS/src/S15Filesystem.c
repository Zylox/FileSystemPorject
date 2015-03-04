#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <VirtualBlockStorage.h>

#include "../include/S15Filesystem.h"

#define VBS_FILENAME "HARD_DRIVE"

#define INODE_NUM 256

#define FS_FIRST_BLOCK_IDX 40

#define MOUNT_FAILURE -1

/*
 * Gives accesses to the VBS for all functions inside this .c
 * only. The fs_mount function will provide VBS structure
 * for usage with the vbs API functions
 * */
static VBS_Type *virtualBlockStorage;


static Inode_t inodes[INODE_NUM];

// static unsigned char firstOpenInodeIdx(){
	
	// int i;
	////start at one because the first inode is the root always;
	// for(i=1; i < INODE_NUM; i++){
		// if(inodes[i].metaData.fileLinked == 0){
			// return (unsigned char) i;
		// }
	// }
	
	// return 0;
// }

// static char* getDirectorysFromPath(const char* absolutePath){
	
	// char* afnCopy = strdup(absolutePath);
	// char* tokens = strtok(afnCopy, "/");
	// char* directoryPath;
	
	// while (tokens) {
		// tokens = strtok(NULL, " ");
		
	// }
// }

static unsigned int calculateFileSize(Inode_t file){
	unsigned int size = 0;
	return size;
	
}

int fs_mount() {
	
	vbs_initialize(VBS_FILENAME);
	virtualBlockStorage = vbs_open(VBS_FILENAME);	
	
	BlockType loader = vbs_make_block();
	Inode_t rootInode;
	
	strncpy(rootInode.fileName, "root", sizeof("root"));
	rootInode.metaData.fileType = DIR_FILE;
	rootInode.metaData.fileSize = sizeof(Directory_t);
	rootInode.metaData.fileLinked = FILE_LINKED;
	rootInode.blockPointers[0] = FS_FIRST_BLOCK_IDX;
	inodes[0] = rootInode;
	
	Directory_t rootDir;
	rootDir.size = 0;
	FS_Block_t rootBlock;
	rootBlock.validBytes = sizeof(Directory_t);
	rootBlock.nextBlockIdx = 0;
	memcpy(&(rootBlock.dataBuffer), &rootDir, sizeof(Directory_t));
	
	short error = -1;
	
	memcpy(loader.buffer, &rootBlock, sizeof(FS_Block_t));
	
	VBS_Index idx = vbs_nextFreeBlock(virtualBlockStorage);
	printf("%u is the next open block\n",idx);
	error = vbs_write(virtualBlockStorage, idx, loader);
	if(error < 0){
		perror("error in creating root dir");
		return MOUNT_FAILURE;
	}
	
	
	int i;
	int inodeLoaderOffset=0;
	error = -1;
	for(i = 1; i < INODE_NUM; i++){
		inodes[i].metaData.fileLinked = FILE_UNLINKED;
	}
	for(i = 8; i <=39; i+=1){
		
		memcpy(loader.buffer, &inodes[inodeLoaderOffset], sizeof(BlockType));
		//8 inodes per blocktype;
		inodeLoaderOffset+=8;
		
		error = vbs_write(virtualBlockStorage, i, loader);
		if(error < 0){
			perror("error in the vbs write");
			return MOUNT_FAILURE;
		}
	}

	return 0;
}

int fs_create_file(const char* absoluteFilename,FileType fileType) {

	//find first open inode;
	// unsigned char openIdx = firstOpenInodeIdx();
	// if(openIdx == 0){
		// perror("no open inodes");
		// return -1;
	// }
	
	/*typedef struct {
		char fileName[64];
		FileMetadata_t metaData;
		unsigned short blockPointers[8];
	} Inode_t;*/
	
	// Inode_t newInode;
	// char* afnCopy = strdup(absoluteFilename);
	// char* tokens = strtok(afnCopy, "/");
	
	// while (token) {
		
		// token = strtok(NULL, " ");
	// }
	// strncpy(newInode.filename, , 64);
	
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


