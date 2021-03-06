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
#define FILE_ACCESS_FAILURE -7

/*
 * Gives accesses to the VBS for all functions inside this .c
 * only. The fs_mount function will provide VBS structure
 * for usage with the vbs API functions
 * */
static VBS_Type *virtualBlockStorage;


static Inode_t inodes[INODE_NUM];

static void initDir(Directory_t* dir){
	dir->entries = malloc(DIR_E_T_SIZE*10);
	dir->size = 0;
}

static int addDirectoryEntry(Directory_t* dir, const char* filename, unsigned char inodeIdx){
	if(dir->size >= DIR_E_T_SIZE*10){
		perror("directory is full");
		return DIRECTORY_FULL;
	}
	
	if(strlen(filename) > MAX_FILENAME_LEN){
		perror("Name too long, shorten to less than MAX_FILENAME_LEN");
		return INVALID_PATH;
	}
	DirectoryEntry_t dirE;
	memcpy(dirE.filename, filename, sizeof(char)*MAX_FILENAME_LEN+1);
	dirE.inodeIdx = inodeIdx;
	
	memcpy(dir->entries + dir->size, &dirE, DIR_E_T_SIZE);
	dir->size += DIR_E_T_SIZE;
	
	return 1;
}

static int removeDirectoryEntry(Directory_t* dir, int index){
	if(index > 9){
		perror("directory index out of scope");
		return -1;
	}
	
	DirectoryEntry_t* dirE = dir->entries;
	dirE += index;
	dir->size -= DIR_E_T_SIZE;
	if(index < 9){
		memcpy(dirE, dirE+1, DIR_E_T_SIZE * (9-index));
	}
	
	return 1;
}

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
	DirectoryEntry_t* entries = malloc(DIR_E_T_SIZE*10);
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
	int error = -1;
	
	BlockType loader = vbs_make_block();
	
	for(i = 8; i <=39;i++){
		for(j=0;j<8;j++){
			memcpy(loader.buffer + (j * INODE_T_SIZE), packInode(&inodes[i+j]), INODE_T_SIZE);
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
				Directory_t tDir = unpackDirectory(fsBlock.dataBuffer);
				newDir = &tDir;
				
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

	short error = -1;
	
	Directory_t rootDir;
	initDir(&rootDir);
	error = addDirectoryEntry(&rootDir, rootInode.fileName, 0);
	if(error < 1){
		perror("Initilizing the directory fialed");
		return MOUNT_FAILURE;
	}
	FS_Block_t rootBlock;
	rootBlock.validBytes = DIR_T_SIZE;
	rootBlock.nextBlockIdx = 0;
	memcpy(&(rootBlock.dataBuffer), packDirectory(&rootDir), DIR_T_SIZE);
	

	
	memcpy(loader.buffer, packFSBlock(&rootBlock), FS_BLOCK_T_SIZE);
	
	error = vbs_write(virtualBlockStorage, FS_FIRST_BLOCK_IDX, loader);
	if(error < 0){
		perror("error in creating root dir");
		return MOUNT_FAILURE;
	}
	
	int i;
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
		nextToken = strtok(NULL, "/");
	}
	int filenameSize = strlen(token);
	
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

	
	
	if(strlen(filename) > MAX_FILENAME_LEN){
		perror("filename too long");
		return INVALID_PATH;
	}

	
	
	Directory_t dir;
	error = fs_get_directory(dirPath, &dir);
	if(error < 1){

		return DIRECTORY_NOT_FOUND;
	}

	
	int lastDirEntryIdx = dir.size/DIR_E_T_SIZE;
	if(lastDirEntryIdx > 10){
		perror("Directory is full, no more files can be added");
		return DIRECTORY_FULL;
	}

	
	Inode_t file;
	strncpy(file.fileName, filename, strlen(filename));
	file.metaData.fileType = fileType;
	file.metaData.fileSize = 0;
	file.metaData.fileLinked = FILE_LINKED;
	
	unsigned char inodeIdx = firstOpenInodeIdx();
	if(inodeIdx == NO_OPEN_INODE){
		return FILE_CREATION_FAILURE;
	}

	
	error = addDirectoryEntry(&dir, filename, inodeIdx);
	if(error < 1){
		return error;
	}
	
	inodes[inodeIdx] = file;
	
	error = writeInodeTable();
	if(error < 1){
		perror("inodewritefailedsomehow");
		return error;
	}

	
	free(dirPath);
	free(filename);

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
	while (tokens) {
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
	//ran out of time
	return 0;
}

static int eraseFile(Inode_t inode){
	BlockType vbsBlock= vbs_make_block();	
	FS_Block_t fsBlock;
	unsigned short nextBID = inode.blockPointers[0];
	bool erased;
	
	do{
		vbsBlock = vbs_read(virtualBlockStorage, nextBID);	
		fsBlock = unpackFSBlock(vbsBlock.buffer);
		erased = vbs_erase(virtualBlockStorage, nextBID);
		if(erased == false){
			perror("failure erasing file");
			return FILE_ACCESS_FAILURE;
		}
		nextBID = fsBlock.nextBlockIdx;
	}while(nextBID != 0);
	
	return 1;
}

int fs_remove_file(const char* absoluteFilename) {
	char *dirPath, *filename;
	int error = -1;
	error = splitFileAndDirPath(absoluteFilename, &dirPath, &filename);
	if(error < 1){
		return INVALID_PATH;
	}
	
	Directory_t dir;
	error = fs_get_directory(dirPath, &dir);
	if(error < 1){
		return DIRECTORY_NOT_FOUND;
	}
	
	if(dir.size != 0){
		//design decision
		perror("cannot delete directory with items");
		return INVALID_PATH;
	}
	
	DirectoryEntry_t* dirEntry;
	dirEntry = dir.entries;
	int i;
	for(i = 0; i < 10; i++){
		if(strcmp(filename, (*dirEntry).filename)){
			Inode_t inode = inodes[(*dirEntry).inodeIdx];
			inode.metaData.fileLinked = FILE_UNLINKED;
			error = eraseFile(inode);
			if(error < 1){
				return error;
			}
			error = removeDirectoryEntry(&dir, i);
			if(error < 1){
				return error;
			}
			return 1;
		}
		dirEntry++;
	}
	
	printf("file does not exist\n");
	return 1;
}


int fs_write_file(const char* absoluteFilename, void* dataToBeWritten, unsigned int numberOfBytes) {
	// char *dirPath, *filename;
	// int error = -1;
	// error = splitFileAndDirPath(absoluteFilename, &dirPath, &filename);
	// if(error < 1){
		// return INVALID_PATH;
	// }
	
	// Directory_t dir;
	// error = fs_get_directory(dirPath, &dir);
	// if(error < 1){
		// return DIRECTORY_NOT_FOUND;
	// }
	
	// DirectoryEntry_t* dirEntry;
	// dirEntry = dir.entries;
	// int i;
	// for(i = 0; i < 10; i++){
		// if(strcmp(filename, (*dirEntry).filename)){
			// Inode_t inode = inodes[(*dirEntry).inodeIdx];
			// FS_Block_t fsBlock;
			// BlockType vbsBlock= vbs_make_block();	
			// unsigned int bytesWritten=0;
			// unsigned short nextOpenFSB;
			// nextOpenFSB = vbs_nextFreeBlock(virtualBlockStorage);
			// fsBlock.nextBlockIdx = nextOpenFSB;
			// for(;;){
				
				// memcpy(fsBlock.dataBuffer, dataToBeWritten + bytesWritten,sizeof(char)*1020);
				// bytesWritten += 1020;
				// fsBlock.validBytes=1020;
				// memcpy(vbsBlock.buffer, packFSBlock(&fsBlock), FS_BLOCK_T_SIZE);
				////crap, i need another open one inddex and it wont return a new one till i write to this on.......
				////im out of time unfortuantley....
			// }
			
		// }
		// dirEntry++;
	// }
	
	
	
	return 0;
}

int fs_read_file(const char* absoluteFilename, void* dataToBeRead, unsigned int numberOfBytes) {
	//ran out of time
	return 0;
}

/*
 * Doesn't truncate the file, just closes the filesystem access
 * to the vbs file used.
 * */
int fs_unmount() {
	return vbs_close(virtualBlockStorage);
}


