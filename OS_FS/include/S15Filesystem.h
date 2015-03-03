#ifndef _S15_FILESYSTEM_H
#define _S15_FILESYSTEM_H

#define MAX_FILENAME_LEN 63
#define FILE_LINKED 'y'
#define FILE_UNLINKED 'n'

typedef enum FILE_OPTIONS {REG_FILE,DIR_FILE} FileType;

typedef struct {
	char filename[MAX_FILENAME_LEN+1];  // +1 for the \0 
	unsigned char inodeIdx; //0 - 255 possible inodes, per specs implicitly
} DirectoryEntry_t;

typedef struct {
	DirectoryEntry_t * entries;
	size_t size;
} Directory_t;

//needs to be 48 bytes
typedef struct {
	FileType fileType; //1
	unsigned int filesize; //4
	char fileLinked; //1
	//time_t timeCreated; //4 maybe
	//time_t timeModified; //4 maybe
	//time_t timeAccessed; //4 maybe
	//char filler[42];
} FileMetadata_t;

typedef struct {
	char fileName[64];
	FileMetadata_t metaData;
	unsigned short blockPointers[8];
} Inode_t;

typedef struct {
	unsigned short validBytes;
	char dataBuffer[1020];
	unsigned short nextBlockIdx;
} FS_Block_t;






/**
 * Makes the VBS available for use 
 */
int fs_mount(); 

/**
 * Closes the VBS
 */
int fs_unmount();

/*
 * Check Project Specs for Definitions
 */

/**
 * TODO: Fill me in with some Doxygen / JavaDoc style comments
 */
int fs_create_file(const char* absoluteFilename, FileTypes fileType);

/**
 * TODO: Fill me in with some Doxygen / JavaDoc style comments
 */
int fs_get_directory (const char* absolutePath, Directory_t* directoryContents); 

/**
 * TODO: Fill me in with some Doxygen / JavaDoc style comments
 */
int fs_seek_within_file (const char* absoluteFilename, unsigned int offset, unsigned int orgin);

/**
 * TODO: Fill me in with some Doxygen / JavaDoc style comments
 */
int fs_remove_file(const char* absoluteFilename);

/**
 * TODO: Fill me in with some Doxygen / JavaDoc style comments
 */
int fs_write_file(const char* absoluteFilename, void* dataToBeWritten, unsigned int numberOfBytes);

/**
 * TODO: Fill me in with some Doxygen / JavaDoc style comments
 */
int fs_read_file(const char* absoluteFilename, void* dataToBeRead, unsigned int numberOfBytes);

#endif	/* END _S15_FILESYSTEM_H */

