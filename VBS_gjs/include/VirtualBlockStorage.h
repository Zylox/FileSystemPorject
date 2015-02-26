#ifndef __VBS_VirtualBlockDevice_H_
#define __VBS_VirtualBlockDevice_H_

#define VBS_BLOCK_COUNT 65536

#define VBS_BLOCK_SIZE 1024

#include <stdbool.h>
#include "bitmap.h"

/**
	Virtual Block Device (VBS) driver library.

	A VBS is a single file that can be used as 2nd-ary 
	storage device concept, permitting management of
	byte blocks

	Note that the VBS here is constructed similar to what you see with IO Streams using FILE*
	For reference, read: http://www.cplusplus.com/reference/cstdio/FILE/

	- GJS

	NOTE: The BM structure I use in memory is slightly more structured and 
		re-uses the independent/stand-alone bitmap code


	VBS
	 ||---> bm ; FBM in memory
	 |	||--> bits [] ; array 
	 |	|---> size ; num bits 
	 |
	 |----> fd ; // File Descriptor
	
*/
typedef struct VBS_Type { 
	bitmap_t bm; 	// The free block map
	int fd;		// The file descriptor
} VBS_Type;

typedef unsigned short VBS_Index;

/**
	Per specs, blocks are 1KB = 1024 Bytes
*/


/**
	The BlockType 	
	Have to keep in mind, this typedef just a named uchar*
*/
typedef struct BlockType {
	unsigned char * buffer;	// we will use the API to ensure : unsigned char[1024]
} BlockType; 






/**
	Initialize a file to be a VBS
	64 MB

	\param filename The file that will be created in the true FS
	
	\returns true if the file was create and initialized
*/
bool vbs_initialize(const char * filename);

/**
	Open a VBS for usage
	
	\param filename The file that will be created in the true FS

	\returns A pointer to a VBS, or NULL on error
*/
VBS_Type * vbs_open(const char * filename);

/**
	Close a VBS and possibly sync to disk

	\param vbs The VBS to object flush and deallocate

	\returns true if the VBS was synced and file was closed 
*/
bool vbs_close(VBS_Type * vbs);

/**
	Find the next free block index
	\param vbs The VBS to object flush and deallocate

	\returns next free block index, or 0 on error
*/
VBS_Index vbs_nextFreeBlock(VBS_Type * vbs);

/**
	Read a block of data

	\param vbs The VBS to to read
	\param bId Block index in the VBS

	\returns The block of data, or on error we return a BT with a NULL buffer
*/
BlockType vbs_read(VBS_Type * vbs, VBS_Index bId);

/**
	Write a block of data

	\param vbs The VBS to object to write
	\param bId Block index in the VBS
	\param block the actual block that is to be written

	\returns Number of bytes written, or < 0 on error
*/
short vbs_write(VBS_Type * vbs, VBS_Index bId, BlockType block);

/**
	Erases the specified block (frees it) from the storage

	\param vbs The VBS to object to write
	\param bId Block index in the VBS

	\returns true if the block was erased, false on erro
*/
bool vbs_erase(VBS_Type * vbs, VBS_Index bId);


// ======================= BlockType METHODS DECLARATION ==============


/**
	Release a block back.
	\param block the actual block that is to be written

	\returns true if the block (buffer mem) was released
*/
bool vbs_release_block(BlockType block);

/**
	Allocate block for data shuffling, initialized to 0 bytes.

	\returns the allocated block reference, or BlockType.buffer == NULL on error
*/
BlockType vbs_make_block();

#endif /* __VBS_VirtualBlockDevice_H_ */
