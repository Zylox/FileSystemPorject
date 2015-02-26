#include "../include/VirtualBlockStorage.h"

#include <unistd.h>	// syscall ops
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>	
#include <errno.h>	// perror
#include <string.h>	// Mem ops
#include <strings.h>  	// Oh yeah!, bit functions that are probably hardwired to the ISA ops

#include <sys/types.h>
#include <sys/stat.h>

// ======================= PRIVATE METHODS DECLARATION ==============
//		Not exposed in header
int _vbs_write_fbm(VBS_Type * vbs);
BlockType vbs_error_block();


// ======================= VBS API METHODS IMPLEMENTATION ==============

bool vbs_initialize(const char * filename)
{
	// Create / Truncate the file to 64MB
	int tmpFD = open(filename, O_TRUNC | O_CREAT | O_WRONLY , 0644 );
	if (-1 == tmpFD) 
	{
	    perror ("vbs_initialize: Failed to initialize VBS device file");
	    return false;
	}

	// Allocate the full file size:
	// Bytes = VBS_BLOCK_SIZE 1024  * VBS_BLOCK_COUNT 65536
	if (0 != ftruncate(tmpFD,VBS_BLOCK_SIZE * VBS_BLOCK_COUNT))
	{
	    perror ("vbs_initialize: Failed truncate VBS device");
	    return false;
	}

	// bitmap create(size_t size);
	bitmap_t bm = bm_create(VBS_BLOCK_COUNT);
	if (bm.bits == NULL || bm.size != VBS_BLOCK_COUNT)
	{
	    perror ("vbs_initialize: Failed to initialize VBS device, bad structure");
	    return false;
	}

	// We compute the FBM takes 8 blocks, so the first byte = FF
	bm_set(bm,0);
	bm_set(bm,1);
	bm_set(bm,2);
	bm_set(bm,3);
	bm_set(bm,4);
	bm_set(bm,5);
	bm_set(bm,6);
	bm_set(bm,7);

	// write to file 
	const size_t bitmapBytes = VBS_BLOCK_COUNT / BITS_PER_BYTE;  // #blocks = # bits  / bits/byte = #bytes
	ssize_t check;
	if ((check = write(tmpFD, (void *) bm.bits, bitmapBytes)) != bitmapBytes)
	{
	    perror ("vbs_initialize: Failed to initialize VBS device with the bitmap");
	    bm_destroy(&bm);
	    return false;
	}

	if (0 != close	(tmpFD))
	{
	    perror ("vbs_initialize: Failed to cleanly close the initialized VBS");
	    bm_destroy(&bm);
	    return false;
	}

	bm_destroy(&bm);
	return true;
}


VBS_Type * vbs_open(const char * filename)
{

	// Allocate our VBS object, note size of structure not pointer
	// note I am dereferencing one of my pointers
	VBS_Type * vbs = (VBS_Type *) malloc(sizeof(VBS_Type) );

	if (vbs == NULL)
	{
	    perror ("vbs_open: Failed to allocate VBS object");
	    return NULL;
	}

	// Open the VBS and set up the file descriptor
	vbs->fd = open(filename,  O_RDWR , 0644 );
	if (-1 == vbs->fd) {
	    perror ("Failed to Open VBS device");
	    free(vbs);
	    return NULL;
	}

	// We allocated a pointer above, need to get a stack copy copied in... the bits are on the heap
	vbs->bm = bm_create(VBS_BLOCK_COUNT);
	if (vbs->bm.bits == NULL || vbs->bm.size != VBS_BLOCK_COUNT)
	{
	    perror ("vbs_open: Failed to initialize VBS device FBM");
	    free(vbs);
	    return NULL;
	}

	// Read FBM bits from file
	const size_t bitmapBytes = VBS_BLOCK_COUNT / BITS_PER_BYTE;
	ssize_t check;
	if ((check = read(vbs->fd, (void *) vbs->bm.bits, bitmapBytes)) != bitmapBytes)
	{
	    perror ("vbs_open: Failed to initialize VBS device with the bitmap");
	    bm_destroy(&(vbs->bm));
	    free(vbs);
	    return NULL;
	}

	return vbs;
}


bool vbs_close(VBS_Type * vbs)
{
	// Flush the bitmap to the file
	if (vbs != NULL)
	{
	   _vbs_write_fbm(vbs);
	} 
	else 
	{
	    perror ("vbs_close: Invalid VBS Object");
	    return false;
	}

	// Destroy the in-memory FBM
	bm_destroy(&(vbs->bm));

	// close file
	if (0 != close(vbs->fd))
	{
	    perror ("vbs_close: Failed to cleanly close VBS");
	    return false;
	}

	// remove the struct
	free(vbs);
	
	return true;
}


VBS_Index vbs_nextFreeBlock(VBS_Type * vbs)
{
	// Check valid VBS
	if (vbs == NULL)
	{
	    perror ("vbs_nextFreeBlock: Invalid VBS Object");
	    return 0; // this is the error condition, as we cannot write that block ... it holds the FBM
	}

#if 0
	// Iterate through the bytes in the BM until we find an open block
	// Honestly, it is better to use unsigned long or long long so we are doing 8- or 16-byte chunks
	// Then, simple : TZCNT  reg64 reg/mem64 F3 0F BC /r Count the number of trailing zeros in reg/mem64.
	// but for clarity
	const size_t bitmapBytes = VBS_BLOCK_COUNT / BITS_PER_BYTE;  // #blocks = # bits  / bits/byte = #bytes
	unsigned char testMe = 0xFF;
	int i=0;
	for ( ; i < bitmapBytes; ++i)
	{
		testMe = vbs->bm.bits[i];
		if (0xFF == (0xFF & testMe))
			continue; // nothing to see here, moving on
		else
			break;	// found 
	}
	// i is the block group, testMe has the 0 bit someplace
	// flip the bits, run the FFS algorithm from glibc
	int whichBitIndex = ffs((int) ~testMe);
	if (whichBitIndex == 0)
	{
	    perror ("vbs_nextFreeBlock: No free block found");
	    return 0; // this is the error condition, as we cannot write that block ... it holds the FBM
	}
#endif 

	// Iterate through 32-bit words in the BM until we find an open block
	// Note we have : ffsl() and ffsll() functions are glibc extensions, if we want to lose portability. 
	const size_t bitmap32Words = VBS_BLOCK_COUNT / (BITS_PER_BYTE * sizeof(int)); 
	int testMe = 0xFFFFFFFF;
	int i=0;
	int * words = (int *)vbs->bm.bits;	// cast to a word array
	for (;i<bitmap32Words;++i)	// Find first word that has an open bit
	{
		testMe = words[i];
		if (0xFFFFFFFF == (0xFFFFFFFF & testMe))
		continue; // nothing to see here, moving on
		else
			break;	// found 
	}
	// We ran off the end
	if (i==bitmap32Words)
		return 0;

	int whichBitIndex = ffs(~testMe);
	// we have the byte index i and the bit inside whichBitIndex 
	return (VBS_Index) ( (i *  BITS_PER_BYTE * sizeof(int)) + whichBitIndex - 1);
}

// ======================= VBS Block I/O METHODS IMPLEMENTATION ==============

BlockType vbs_read(VBS_Type * vbs, VBS_Index bId)
{ 
	// NOTE: On error, we return a BT with a NULL buffer
	//    BlockType error;
	//    error.buffer = NULL;
	//    return error;

	// Check valid VBS
	if (vbs == NULL)
	{
	    perror ("vbs_read: Invalid VBS Object");
	    return vbs_error_block();
	}

	// is this a valid index
	const size_t dataStartIndex =  (VBS_BLOCK_COUNT / BITS_PER_BYTE) / VBS_BLOCK_SIZE ; // 8 - Blocks ?
	if (dataStartIndex > bId)
	{
	    perror ("vbs_read: Invalid Read Request, not a data block");
	    return vbs_error_block();
	}

	// Is this a valid data block?
	if (! bm_isSet(vbs->bm,bId))
	{
	    perror ("vbs_read: Invalid Read Request, not a previously written data block");
	    return vbs_error_block();
	}

	// allocate and init to zero a buffer
	BlockType bt =  vbs_make_block();

	// Check valid Block
	if (bt.buffer == NULL)
	{
	    perror ("vbs_read: Failed to allocate a block for data");
	    return vbs_error_block();
	}

	// Compute the storage offset and reposition, then read
	const size_t offset = bId * VBS_BLOCK_SIZE;  
	ssize_t check;
	lseek(vbs->fd,offset, SEEK_SET);
	if ((check = read(vbs->fd, (void *) bt.buffer, VBS_BLOCK_SIZE)) != VBS_BLOCK_SIZE)
	{
	    perror ("vbs_read: Failed to read full block VBS");
	    vbs_release_block(bt); // dont bleed on errors
	    return vbs_error_block();
	}	

	return bt; // return the wrapped reference to our heap block
}


short vbs_write(VBS_Type * vbs, VBS_Index bId, BlockType block)
{
	int written = -1; // default error
	// Check valid VBS
	if (vbs == NULL)
	{
	    perror ("vbs_write: Invalid VBS Object");
	    return -1;
	}

	// Check valid block
	if (block.buffer == NULL)
	{
	    perror ("vbs_write: NULL buffer in block for data");
	    return -1;
	}


	// is this a valid index
	const size_t dataStartIndex =  (VBS_BLOCK_COUNT / BITS_PER_BYTE) / VBS_BLOCK_SIZE ; // 8 - Blocks ?
	if (dataStartIndex > bId)
	{
	    perror ("vbs_write: Invalid Write Request");
	    return -1;
	}

	// Compute the file offset and reposition, then read
	const size_t offset = bId * VBS_BLOCK_SIZE;  
	lseek(vbs->fd,offset, SEEK_SET);
	if ((written = write(vbs->fd, (void *) block.buffer, VBS_BLOCK_SIZE)) != VBS_BLOCK_SIZE)
	{
	    perror ("vbs_write: Failed to write full block VBS");
	    return -1;
	}	
	
	// update disk and memory version of the FBM
	bm_set(vbs->bm, bId);
	if (0 != _vbs_write_fbm(vbs))
	{
	    perror ("vbs_write: Failed to sync the FBM" );
	    return -1;
	}

	return written;
}


bool vbs_erase(VBS_Type * vbs, VBS_Index bId)
{

	// Check valid VBS
	if (vbs == NULL)
	{
	    perror ("vbs_erase: Invalid VBS Object");
	    return false;
	}

	// is this a valid index
	const size_t dataStartIndex =  (VBS_BLOCK_COUNT / BITS_PER_BYTE) / VBS_BLOCK_SIZE ; // 8 - Blocks ?
	if (dataStartIndex > bId)
	{
	    perror ("vbs_erase: Invalid Erase Request");
	    return false;
	}

	// allocate and init to zero a buffer
	BlockType bt =  vbs_make_block();
	// Check valid block
	if (bt.buffer == NULL)
	{
	    perror ("vbs_erase: NULL buffer prepared");
	    vbs_release_block(bt); // dont bleed on errors
	    return false;
	}

	// Compute the file offset and reposition, then read
	const size_t offset = bId * VBS_BLOCK_SIZE;  
	ssize_t check;
	lseek(vbs->fd,offset, SEEK_SET);
	if ((check = write(vbs->fd, (void *) bt.buffer, VBS_BLOCK_SIZE)) != VBS_BLOCK_SIZE)
	{
	    perror ("vbs_erase: Failed to write full block VBS");
	    vbs_release_block(bt); // dont bleed on errors
	    return false;
	}	

	// We wrote the empty block, now release it
	vbs_release_block(bt);

	// update disk and memory version of the FBM
	bm_unset(vbs->bm,bId);
	_vbs_write_fbm(vbs);
	return true;
}

// ======================= BlockType METHODS IMPLEMENTATION ==============

bool vbs_release_block(BlockType block)
{
	if (block.buffer != NULL)
	{
		free(block.buffer);
	}
	return true;
}

BlockType vbs_make_block()
{
	// allocate and init to zero a buffer
	BlockType bt; // This is now a double pointer
	if (NULL == (bt.buffer = (unsigned char *)calloc(sizeof(unsigned char),VBS_BLOCK_SIZE) ) )
	{
	    perror ("vbs_make_block: Failed to allocate buffer");
	    return bt;	// returns the buffer = NULL on error
	}

	memset((void*)bt.buffer, 0, VBS_BLOCK_SIZE);

	return bt; // return the allocated memory address
}

// ======================= PRIVATE METHODS IMPLEMENTATION ==============
//		Not exposed in header

int _vbs_write_fbm(VBS_Type * vbs)
{
	// rewind and write to file
	const ssize_t bitmapBytes = VBS_BLOCK_COUNT / BITS_PER_BYTE;  // #blocks = # bits  / bits/byte = #bytes
	ssize_t check;
	lseek(vbs->fd,0, SEEK_SET);
	if ((check = write(vbs->fd, (void *) vbs->bm.bits, bitmapBytes)) != bitmapBytes)
	{
	    perror ("Failed to initialize VBS device with the bitmap");
	    return -1;
	}
	
	return 0;
}

BlockType vbs_error_block()
{
	    BlockType error;
	    error.buffer = (unsigned char *) NULL;
	    return error;
}





