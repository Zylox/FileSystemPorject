#ifndef __VBS_BITMAP_H_
#define __VBS_BITMAP_H_

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

/// The structure / object.  This is how Object Oriented is done without objects
typedef struct bitmap_t {  uint8_t * bits; size_t size; } bitmap_t;

#define BITS_PER_BYTE 8

// The allocation / deallocation

/**
 Upon creation, test the return bm size for > 0
 If size = 0, then error;
 \param size The number of bits in the bitmap
*/
bitmap_t bm_create(size_t size);

/**
 Destroy the bitmap, reset size to 0 and free memory
 \param bm The bitmap object
*/
void bm_destroy(bitmap_t * bm);

// Ops to inspect or manipuate the data type
// 0-based indexing because ... Computer Science

/**
 Test the bit is set (true) or unset (false)
 \param bm The bitmap object
 \param bitId Which actual Bit in the map to check
*/
bool bm_isSet(bitmap_t bm, size_t bitId );

/**
 Set the bit
 \param bm The bitmap object
 \param bitId Which actual Bit in the map to set
*/
void bm_set(bitmap_t bm, size_t bitId );

/**
 UnSet the bit
 \param bm The bitmap object
 \param bitId Which actual Bit in the map to unset
*/
void bm_unset(bitmap_t bm, size_t bitId );

#endif /* BITMAP */
