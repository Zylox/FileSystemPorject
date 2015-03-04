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

#define NO_OPEN_INODE 0
#define MOUNT_FAILURE -1
#define INVALID_PATH -2
#define DIRECTORY_NOT_FOUND -3
#define INCORRECT_FILE_TYPE -4
#define DIRECTORY_FULL -5
#define FILE_CREATION_FAILURE -6

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
		if(inodes[i].metaData.fileLinked == 0){
			return (unsigned char) i;
		}
	}
	
	perror("all inodes in use");
	return NO_OPEN_INODE;
}

static int writeInodeTable(){
	int i;
	int inodeLoaderOffset=0;
	int error = -1;
	
	BlockType loader = vbs_make_block();
	
	for(i = 8; i <=39; i+=1){
		
		memcpy(loader.buffer, &inodes[inodeLoaderOffset], sizeof(BlockType));
		//8 inodes per blocktype;
		inodeLoaderOffset+=8;
		
		error = vbs_write(virtualBlockStorage, i, loader);
		if(error < 0){
			perror("error in the vbs write of inode table");
			return error;
		}
	}
	
	return 1;
}

static Directory_t getRootDirectory(){
	Inode_t workingFile = inodes[0];
	BlockType vbsBlock= vbs_make_block();
	vbsBlock = vbs_read(virtualBlockStorage, workingFile.blockPointers[0]);
	FS_Block_t fsBlock;
	memcpy(&fsBlock,vbsBlock.buffer,sizeof(FS_Block_t));
	
	Directory_t dir;
	memcpy(&dir,fsBlock.dataBuffer,sizeof(Directory_t));
	
	return dir;
}

static int getDirectoryFromToken(const char* dirName, Directory_t currentDir, Directory_t* newDir){
	DirectoryEntry_t* dirEntry;
	dirEntry = currentDir.entries;
	int i;
	for(i = 0; i < 10; i++){
		if(strcmp(dirName, (*dirEntry).filename)){
			Inode_t file = inodes[inodeIdx];
			if(file.metaData.fileType == DIR_FILE){
				BlockType vbsBlock= vbs_make_block();
				vbsBlock = vbs_read(virtualBlockStorage, file.blockPointers[0]);
				FS_Block_t fsBlock;
				memcpy(&fsBlock,vbsBlock.buffer,sizeof(FS_Block_t));

				memcpy(newDir,fsBlock.dataBuffer[0],sizeof(Directory_t));
				return 1;
			}else{
				perror("dirname is a file not a directory");
				return INCORRECT_FILE_TYPE;
			}
		}
	}
	
	return DIRECTORY_NOT_FOUND
}

// static char* getDirectorysFromPath(const char* absolutePath){
	
	// char* afnCopy = strdup(absolutePath);
	// char* tokens = strtok(afnCopy, "/");
	// char* directoryPath;
	
	// while (tokens) {
		// tokens = strtok(NULL, " ");
		
	// }
// }

// static unsigned int calculateFileSize(Inode_t file){
	// unsigned int size = 0;
	// return size;
	
// }

int fs_mount() {
	
	vbs_initialize(VBS_FILENAME);
	virtualBlockStorage = vbs_open(VBS_FILENAME);	
	
	BlockType loader = vbs_make_block();
	Inode_t rootInode;
	
	strncpy(rootInode.fileName, "/", sizeof("/"));
	rootInode.metaData.fileType = DIR_FILE;
	rootInode.metaData.fileSize = sizeof(Directory_t);
	rootInode.metaData.fileLinked = FILE_LINKED;
	rootInode.blockPointers[0] = FS_FIRST_BLOCK_IDX;
	inodes[0] = rootInode;
	
	Directory_t rootDir;
	rootDir.size = sizeof(DirectoryEntry_t);
	FS_Block_t rootBlock;
	rootBlock.validBytes = sizeof(Directory_t);
	rootBlock.nextBlockIdx = 0;
	memcpy(&(rootBlock.dataBuffer), &rootDir, sizeof(Directory_t));
	
	short error = -1;
	
	memcpy(loader.buffer, &rootBlock, sizeof(FS_Block_t));
	
	error = vbs_write(virtualBlockStorage, FS_FIRST_BLOCK_IDX, loader);
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

static int splitFileAndDirPath(const char* absoluteFilename, char* dirPath, char* filename){
	int error=-1;
	
	if(*absoluteFilename != '/'){
		perror("please start filepath with /");
		return INVALID_PATH;
	}
	
	char* afnCopy = strdup(absoluteFilename);
	char* token = strtok(afnCopy, "/");
	char* nextToken;
	Directory_t dir = get_rootDirectory();
	
	while (nextToken){
		token = nextToken;
		nextToken = strtok(NULL, "/");
	}
	int filenameSize = strlen(token);
	dirPath = strncpy(dirPath, absoluteFilename, strlen(absoluteFilename) - filenameSize+1);
	char* dirPathEnd = dirPath + strlen(dirPath)-1;
	*dirPathEnd = '\0';
	filename = token;
	
	return 1;
}

int fs_create_file(const char* absoluteFilename,FileType fileType) {
	int error = -1;
	char* dirPath, filename;
	error = splitFileAndDirPath(absoluteFilename, dirPath, filename);
	if(error < 1){
		return INVALID_PATH;
	}
	
	if(strlen(filename) > MAX_FILENAME_LEN){
		perror("filename too long");
		return INVALID_PATH;
	}
	
	
	Directory_t dir;
	error = fs_get_directory(dirPath, &dir);
	if(error < 1){
		return DIRECTORY_NOT_FOUND;
	}
	
	int lastDirEntryIdx = dir.size/sizeof(DirectoryEntry_t);
	if(lastDirEntryIdx > 10){
		perror("Directory is full, no more files can be added");
		return DIRECTORY_FULL;
	}
	
	Inode_t file;
	strncpy(file.filename, filename, strlen(filename));
	file.metaData.fileType = fileType;
	file.metaData.fileSize = 0;
	file.metaData.fileLinked = FILE_LINKED;
	
	unsigned char inodeIdx = firstOpenInodeIdx();
	if(inodeIdx == NO_OPEN_INODE){
		return FILE_CREATION_FAILURE;
	}
	
	DirectoryEntry_t dirEntry;
	strncpy(dirEntry.filename, filename, strlen(filename));
	dirEntry.inodeIdx = inodeIdx;
	DirectoryEntry_t* insertionPoint = dir.entries+(lastDirEntryIdx);
	*insertionPoint = dirEntry;
	dir.size += sizeof(DirectoryEntry_t);
	
	inodes[inodeIdx] = file;
	
	error = writeInodeTable();
	if(error < 1){
		return error;
	}
	
	return 0;
}





int fs_get_directory (const char* absolutePath, Directory_t* directoryContents) {
	
	int error=-1;
	
	if(*absolutePath != '/'){
		perror("please start filepath with /");
		return INVALID_PATH;
	}
	
	char* afnCopy = strdup(absolutePath);
	char* tokens = strtok(afnCopy, "/");
	
	Directory_t dir = get_rootDirectory();
	
	while (tokens) {
		
		if(tokens == NULL){
			return 1;
		}
		//putting the result in directoryContents then moving it into dir
		// gives the side effect of even on an incomplete return, the most
		// complete filepath possible is given.
		error = getDirectoryFromToken(tokens, dir, directoryContents);
		if(error < 1){
			return error;
		}else{
			dir = *directoryContents;
		}
		
		//printf("path line for %s: %s\n", absolutePath,tokens);
		tokens = strtok(NULL, "/");
		
	}
	
	
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


