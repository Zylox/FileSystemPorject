#include "../include/VirtualBlockStorage.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define PASS_OK printf("\n[%d]\t%s",(++PASSES),"."); 
#define PASS_NOTOK printf("\n[%d]\t%s",(++PASSES),"FAIL");

int main() {

    int PASSES = 0;

	// =============================================
	// Are we able to create the underlying file 
	// that mimics a block storage device
	// =============================================
    assert(
	vbs_initialize("testfile")
	);
// 1
PASS_OK

	// =============================================
	// This is the handle for the VBS
	// Lets try to open it
	// =============================================
    VBS_Type * testVBS = vbs_open("testfile");
    assert(
	testVBS != NULL
	);
// 2
PASS_OK

	// =============================================
	// A block index variable
	// =============================================
    VBS_Index idx;

	

	// =============================================
    	// check FBM
	// Normal usage does not reach into the VBS* 
	// We expect the first 8 blocks are used by the
	// FBM, with is bits [0->7] in FBM
	// =============================================
    assert(testVBS->bm.bits[0] == 0xFF);
// 3
PASS_OK

	// The rest of the data is blank
    for (idx = 1; idx < 8192; ++idx) {
        assert(testVBS->bm.bits[idx] == 0x00);
    }
// 4
PASS_OK

	// =============================================
	// test close and re-open
	// =============================================
    assert(
	testVBS != NULL
	);
// 5
PASS_OK

    testVBS = vbs_open("testfile");

    assert(
	testVBS != NULL
	);
// 6
PASS_OK



	// =============================================
	// create a data block
	// =============================================
    //uint8_t *data = malloc(1024);
    BlockType bt = vbs_make_block();

    if (bt.buffer == NULL) 
    {
        perror("Failed to allocate a block");
        vbs_close(testVBS);
        return -1;
    }
// 7 
PASS_OK


// ------- NOTE: the WAS version uses 0 based indexing starting at first data block
// -------	The GJS version uses 0 based indexing starting at the root of the VBS, which is the FBM


	// =============================================
	// check next_free, RECALL
	// We expect the first 8 blocks are used by the
	// FBM, with is bits [0->7] in FBM
	// =============================================
    assert(
	8 == (idx = vbs_nextFreeBlock(testVBS))
	);
// 8
PASS_OK



	// =============================================
	// check the disaster case, we're full
	// We will do this by hacking the FBM in the VBS
	// 
	// =============================================
    memset(testVBS->bm.bits + 1, 0xFF, 8191);
    assert(
	0 == (idx = vbs_nextFreeBlock(testVBS))
	);
// 9
PASS_OK


	// =============================================
	// Fence-post test
	// also check when last is free, just in case I 
	// messed up the scan
	// (AND I DID)
	// =============================================
    testVBS->bm.bits[8191] = 0X7F;
    assert(
	(VBS_BLOCK_COUNT - 1) == (idx = vbs_nextFreeBlock(testVBS))
	);

// 10
PASS_OK

	// =============================================
	// check a random case by setting some bits off
	// =============================================
    testVBS->bm.bits[10] = 0xA3;
    assert(
	82 ==  (idx = vbs_nextFreeBlock(testVBS))
	);

// 11
PASS_OK



	// =============================================
	// fix the block map, prepare a block... reset to first byte on, rest off
	// =============================================
    memset(testVBS->bm.bits + 1, 0x00, 8191);
	// =============================================
	// Copy data into a buffer
	// =============================================
    memset(bt.buffer, 0xAA, VBS_BLOCK_SIZE);

	// =============================================
	// write block, uhh... 10! (and 0)
	// =============================================
	idx = 10;
    assert(
	-1 != vbs_write(testVBS, idx, bt)
	);

// 12
PASS_OK

	// =============================================
	// Verify the bit is set in the FBM
	// =============================================
    assert(
	bm_isSet(testVBS->bm,idx)
	);


// 13
PASS_OK


	// =============================================
	// Cannot do these test as written, need to read 
	//	from file
	// =============================================
    //assert(testVBS->DATA[10 << 10] == 0xAA);
    //assert(testVBS->DATA[(11 << 10) - 1] == 0xAA);

	BlockType btRead = vbs_read(testVBS, idx);

	for (int b=0;b<VBS_BLOCK_SIZE;++b)
		if (bt.buffer[b] != btRead.buffer[b])
		{
			perror("Read Block does not match written Block");
			PASS_NOTOK
		}

// 14
PASS_OK


	// =============================================
	// Erase the one block we have written
	// =============================================
    assert(
	vbs_erase(testVBS,idx)
	);


// 15
PASS_OK

	//===========================================
	// Verify the bit is now unset in the FBM
	// =============================================
    assert(
	! bm_isSet(testVBS->bm,idx)
	);

// 16
PASS_OK

	
	
	//==============================================
	// Verify The next free block is 8
	// =============================================
    assert(
	8 == (idx = vbs_nextFreeBlock(testVBS))
	);


// 17
PASS_OK
 
	//==============================================
	// Lets write a few blocks and verify the next
	// free block is 8+ num blocks written
	// =============================================

	// idx == 8; // see the above

	BlockType writeTest = vbs_make_block();
    if (writeTest.buffer == NULL) 
    {
	perror("Failed to allocate a write block");
	PASS_NOTOK
	vbs_close(testVBS);
	return 1;
    }

// 18
PASS_OK

	// =============================================
	// Copy data into a buffer
	// =============================================
    memset(writeTest.buffer, 0xFA, VBS_BLOCK_SIZE);

	for (int b= 8 ; b < 23; ++b)
	{
		if (-1 == vbs_write(testVBS, b, writeTest) )
		{
			PASS_NOTOK
			break;
		}
	}
    assert(
	23 == vbs_nextFreeBlock(testVBS)
	);

// 19
PASS_OK


#if 0


    memset(data, 0xFF, 1024);
    assert(VBS_write(testVBS, 0, data) == 0);
    assert(testVBS->FBM[1] == 0x01);
    assert(testVBS->DATA[0] == 0xFF);
    assert(testVBS->DATA[1023] == 0xFF);
    assert(VBS_next_free_block(testVBS) == 1);

#endif


	// =============================================
	// test bad writes
	// =============================================
    //assert(VBS_write(testVBS, 0xFFFF, data) == -1);	// the GJS VBS cannot be index off the end thanks to the compiler

    assert(
	-1 == vbs_write(testVBS, 0, writeTest)
	);

// 20
PASS_OK


	// =============================================
	// test bad reads
	// =============================================
	BlockType badRead = vbs_read(testVBS, 0);

    assert(
	NULL == badRead.buffer	
	);

// 21
PASS_OK


	// =============================================
	//test bad erase
	// =============================================
    assert(
	! vbs_erase(testVBS, 0)
	);


// 22
PASS_OK


    assert(
	vbs_close(testVBS)
	);

// 22
PASS_OK

    remove("testfile"); // comment this if you want to poke around the VBS file

    vbs_release_block(btRead);
    vbs_release_block(bt);
    vbs_release_block(writeTest);

    printf("\n %d Tests Passed!\n", PASSES);
    return 0;
}

