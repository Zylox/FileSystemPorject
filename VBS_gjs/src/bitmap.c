#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/bitmap.h"

#define BITS_PER_BYTE 8

// The structure
// typedef struct bitmap_t {  uint8_t * bits, size_t size} bitmap_t;

// The allocation / deallocation
bitmap_t bm_create(size_t size)
{
	// stack variable
	bitmap_t bm;
	bm.bits = NULL;
	bm.size = size;

	if (size == 0)
	{
		// Early return in empty bitmap
		return bm;
	}

	// allocate memory for the bit map based on size
	// We want the bitmap to be aligned to a byte
	size_t bytes = size / BITS_PER_BYTE;
	if (size % BITS_PER_BYTE > 0) // round up a byte on non-byte aligned bitmap
		++bytes;

	bm.bits = (uint8_t *) calloc(bytes,sizeof(uint8_t));

	if (bm.bits == NULL)
	{
		perror("Error allocating bitmap memory");
		bm.size = 0;
		return bm;		
	}
	else
	{
		// bzero is deprecated, so use memset from string.h
		memset((void *) bm.bits, 0, bytes);
	}
	
	// return the stack variable, which will shallow copy 
	return bm;
}

void bm_destroy(bitmap_t * bm)
{
	if (bm->bits != NULL)
	{
		free (bm->bits);
		bm->bits = NULL;
	}
	bm->size = 0;

	return;
}

// Ops to inspect or manipuate the data type
bool bm_isSet(bitmap_t bm, size_t bitId )
{
	if (bitId >= bm.size)
	{
		perror("Error checking bit out of bounds");
		return false; // or assert?
	}

	if (bm.bits == NULL)
	{
		perror("bm_isSet: Bitmap not initialized");
		return false; // or assert?
	}

	bool setTest = false;
	// Compute the byte, 
	// slice out the byte and use bit shifting 
	// and boolean logic to test the bit
	size_t byteOffset = bitId / BITS_PER_BYTE;
	uint8_t containingByte = bm.bits[byteOffset];
	uint8_t internalBit = bitId % BITS_PER_BYTE;
	setTest = (1 << internalBit) & containingByte;
	// ASSERT isSet(bitmap_t bm, size_t bitId)
	return setTest;
}

void bm_set(bitmap_t bm, size_t bitId )
{
	if (bitId >= bm.size)
	{
		perror("Error checking bit out of bounds");
		return; // or assert?
	}
	
	if (bm.bits == NULL)
	{
		perror("bm_set: Bitmap not initialized");
		return; // or assert?
	}

	// Thank you for my digital logic class
	size_t byteOffset = bitId / BITS_PER_BYTE;
	uint8_t internalBit = bitId % BITS_PER_BYTE;
	bm.bits[byteOffset] |= (1 << internalBit);
}

void bm_unset(bitmap_t bm, size_t bitId )
{
	if (bitId >= bm.size)
	{
		perror("Error checking bit out of bounds");
		return; // or assert?
	}

	if (bm.bits == NULL)
	{
		perror("bm_unset: Bitmap not initialized");
		return; // or assert?
	}

	// Thank you for my digital logic class
	size_t byteOffset = bitId / BITS_PER_BYTE;
	uint8_t internalBit = bitId % BITS_PER_BYTE;
	uint8_t maskByte = (1 << internalBit);
	bm.bits[byteOffset] &= ~maskByte;

	// ASSERT !isSet(bitmap_t bm, size_t bitId)
	return;
}


