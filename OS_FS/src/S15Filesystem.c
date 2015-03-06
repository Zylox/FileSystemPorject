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

#define DIR_E_T_SIZE (sizeof(char)*64 + sizeof(unsigned char))
#define DIR_T_SIZE  ((sizeof(char)*64 + sizeof(unsigned char)) * 10 + sizeof(size_t))
#define INODE_T_SIZE (sizeof(char)*64 + sizeof(FileMetadata_t) + sizeof(unsigned short) * 8)
#define FS_BLOCK_T_SIZE (sizeof(unsigned short) + sizeof(char) * 1020 + sizeof(unsigned short))

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

static char* packDirectoryEntrys(DirectoryEntry_t* dirE){
	char* buffer = malloc(DIR_E_T_SIZE *10);
	int i;
	for(i=0;i<10;i++){
		memcpy(buffer + (i*DIR_E_T_SIZE), dirE->filename, sizeof(char)*64);
		memcpy(buffer + (i*DIR_E_T_SIZE) + sizeof(char)*64, &(dirE->inodeIdx), sizeof(unsigned char));
		dirE++;
	}
	
	return buffer;
}

static DirectoryEntry_t* unpackDirectoryEntrys(char* dataBuffer){
	DirectoryEntry_t* entries;
	DirectoryEntry_t* walker = entries;
	int i;
	for(i=0;i<10;i++){
		memcpy(walker->filename, dataBuffer + (i*DIR_E_T_SIZE), sizeof(char)*64);
		memcpy(&(walker->inodeIdx) ,dataBuffer + (i*DIR_E_T_SIZE) + sizeof(char)*64, sizeof(unsigned char));
		walker++;
	}
	
	return entries;
}

static char* packDirectory(Directory_t* dir){
	char* buffer = malloc(DIR_T_SIZE);
	memcpy(buffer, packDirectoryEntrys(dir->entries), DIR_E_T_SIZE * 10);
	memcpy(buffer + DIR_E_T_SIZE * 10, &(dir->size), sizeof(size_t));
	return buffer;
}

static Directory_t unpackDirectory(char* dataBuffer){
	Directory_t dir;
	dir.entries = unpackDirectoryEntrys(dataBuffer);
	memcpy(&(dir.size), dataBuffer + DIR_E_T_SIZE * 10, sizeof(size_t));
	return dir;
}

static char* packInode(Inode_t* inode){
	char* buffer = malloc(INODE_T_SIZE);
	memcpy(buffer, inode->fileName, sizeof(char) * 64);
	memcpy(buffer + sizeof(char) * 64, &(inode->metaData), sizeof(FileMetadata_t));
	memcpy(buffer + sizeof(char) * 64 + sizeof(FileMetadata_t), inode->blockPointers, sizeof(unsigned short) * 8);
	return buffer;
}

static Inode_t unpackInode(char* dataBuffer){
	Inode_t inode;
	memcpy(inode.fileName, dataBuffer, sizeof(char) * 64);
	memcpy(&(inode.metaData), dataBuffer + sizeof(char) * 64, sizeof(FileMetadata_t));
	memcpy(inode.blockPointers, dataBuffer + sizeof(char) * 64 + sizeof(FileMetadata_t), sizeof(unsigned short) * 8);
	return inode;
}

static char* packFSBlock(FS_Block_t* fsBlock){
	char* buffer = malloc(FS_BLOCK_T_SIZE);
	memcpy(buffer, &(fsBlock->validBytes), sizeof(unsigned short));
	memcpy(buffer + sizeof(unsigned short), fsBlock->dataBuffer, sizeof(char) * 1020);
	memcpy(buffer + sizeof(unsigned short) + sizeof(char) * 1020, &(fsBlock->nextBlockIdx), sizeof(unsigned short));
	return buffer;
}

static FS_Block_t unpackFSBlock(unsigned char* dataBuffer){
	FS_Block_t fsBlock;
	memcpy(&(fsBlock.validBytes), dataBuffer, sizeof(unsigned short));
	memcpy(fsBlock.dataBuffer, dataBuffer + sizeof(unsigned short), sizeof(char) * 1020);
	memcpy(&(fsBlock.nextBlockIdx), dataBuffer + sizeof(unsigned short) + sizeof(char) * 1020, sizeof(unsigned short));
	return fsBlock;
}

static unsigned char firstOpenInodeIdx(){
	
	int i;
	//start at one because the first inode is the root always;
	for(i=1; i < INODE_NUM; i++){
		if(inodes[i].metaData.fileLinked == FILE_UNLINKED){
			return (unsigned char) i;
		}
	}
	
	perror("all inodes in use");
	return NO_OPEN_INODE;
}

static int writeInodeTable(){
	int i, j;
	int inodeLoaderOffset=0;
	int error = -1;
	
	BlockType loader = vbs_make_block();
	
	for(i = 8; i <=39;i++){
		for(j=0;j<8;j++){
			memcpy(loader.buffer + j * INODE_T_SIZE, packInode(inodes[i+j]), INODE_T_SIZE);
		}
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
	FS_Block_t fsBlock = unpackFSBlock(vbsBlock.buffer);
	
	Directory_t dir = unpackDirectory(fsBlock.dataBuffer);
	
	return dir;
}

static int getDirectoryFromToken(const char* dirName, Directory_t currentDir, Directory_t* newDir){
	DirectoryEntry_t* dirEntry;
	dirEntry = currentDir.entries;
	int i;
	for(i = 0; i < 10; i++){
		if(strcmp(dirName, (*dirEntry).filename)){
			Inode_t file = inodes[(*dirEntry).inodeIdx];
			if(file.metaData.fileType == DIR_FILE){
				BlockType vbsBlock= vbs_make_block();
				vbsBlock = vbs_read(virtualBlockStorage, file.blockPointers[0]);
				FS_Block_t fsBlock = unpackFSBlock(vbsBlock.buffer);
				newDir = &(unpackDirectory(fsBlock.dataBuffer));
				return 1;
			}else{
				perror("dirname is a file not a directory");
				return INCORRECT_FILE_TYPE;
			}
		}
		dirEntry++;
	}
	
	return DIRECTORY_NOT_FOUND;
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
	rootInode.metaData.fileSize = DIR_T_SIZE;
	rootInode.metaData.fileLinked = FILE_LINKED;
	rootInode.blockPointers[0] = FS_FIRST_BLOCK_IDX;
	inodes[0] = rootInode;
	
	Directory_t rootDir;
	rootDir.size = sizeof(DirectoryEntry_t);
	FS_Block_t rootBlock;
	rootBlock.validBytes = DIR_T_SIZE;
	rootBlock.nextBlockIdx = 0;
	memcpy(&(rootBlock.dataBuffer), packDirectory(rootDir), DIR_T_SIZE);
	
	short error = -1;
	
	memcpy(loader.buffer, packFSBlock(rootBlock), FS_BLOCK_T_SIZE);
	
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

	writeInodeTable();
	
	return 0;
}

static int splitFileAndDirPath(const char* absoluteFilename, char** dirPath, char** filename){
	if(*absoluteFilename != '/'){
		perror("please start filepath with /");
		return INVALID_PATH;
	}
	
	char* afnCopy = strdup(absoluteFilename);
	char* nextToken = strtok(afnCopy, "/");
	
	char* token;
	
	while (nextToken){
		token = nextToken;
		printf("%s\n", nextToken);
		nextToken = strtok(NULL, "/");
	}
	int filenameSize = strlen(token);
	printf("%d\n",filenameSize);
	*filename = malloc(filenameSize);
	strncpy(*filename, token, strlen(token));
	*dirPath = malloc(strlen(absoluteFilename) - filenameSize+1);
	strncpy(*dirPath, absoluteFilename, strlen(absoluteFilename) - filenameSize+1);
	char* dirPathEnd = *dirPath + strlen(*dirPath)-1;
	*dirPathEnd = '\0';
	
	
	return 1;
}

int fs_create_file(const char* absoluteFilename,FileType fileType) {
	int error = -1;
	char *dirPath, *filename;
	
	error = splitFileAndDirPath(absoluteFilename, &dirPath, &filename);
	if(error < 1){
		return INVALID_PATH;
	}
	printf("7\n");
	
	
	if(strlen(filename) > MAX_FILENAME_LEN){
		perror("filename too long");
		return INVALID_PATH;
	}
	printf("6\n");
	
	
	Directory_t dir;
	error = fs_get_directory(dirPath, &dir);
	if(error < 1){
		printf("5\n");
		return DIRECTORY_NOT_FOUND;
	}

	
	int lastDirEntryIdx = dir.size/sizeof(DirectoryEntry_t);
	if(lastDirEntryIdx > 10){
		perror("Directory is full, no more files can be added");
		return DIRECTORY_FULL;
	}
	printf("4\n");
	
	Inode_t file;
	strncpy(file.fileName, filename, strlen(filename));
	file.metaData.fileType = fileType;
	file.metaData.fileSize = 0;
	file.metaData.fileLinked = FILE_LINKED;
	
	unsigned char inodeIdx = firstOpenInodeIdx();
	if(inodeIdx == NO_OPEN_INODE){
		return FILE_CREATION_FAILURE;
	}
	printf("3\n");
	
	DirectoryEntry_t dirEntry;
	strncpy(dirEntry.filename, filename, strlen(filename));
	dirEntry.inodeIdx = inodeIdx;
	DirectoryEntry_t* insertionPoint = dir.entries+(lastDirEntryIdx);
	*insertionPoint = dirEntry;
	dir.size += sizeof(DirectoryEntry_t);
	
	inodes[inodeIdx] = file;
	
	error = writeInodeTable();
	if(error < 1){
		perror("inodewritefailedsomehow");
		return error;
	}
	printf("2\n");
	
	free(dirPath);
	free(filename);
	
	printf("1\n");
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
	
	Directory_t dir = getRootDirectory();
	*directoryContents = dir;
	printf("sss\n");
	while (tokens) {
		printf("asdfasdfasdf\n");
		if(tokens == NULL){
			return 1;
		}
		//putting the result in directoryContents then moving it into dir
		// gives the side effect of even on an incomplete return, the most
		// complete filepath possible is given.
		error = getDirectoryFromToken(tokens, dir, directoryContents);
		if(error < 1){
			perror("getting directory from token failed");
			return error;
		}else{
			dir = *directoryContents;
		}
		
		//printf("path line for %s: %s\n", absolutePath,tokens);
		tokens = strtok(NULL, "/");
		
	}
	
	
	return 1;


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


