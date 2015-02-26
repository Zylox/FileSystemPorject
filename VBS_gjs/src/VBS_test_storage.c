#include "../include/VirtualBlockStorage.h"
#include <unistd.h>	// syscall ops
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>	
#include <errno.h>	// perror
#include <string.h>	// Mem ops
#include <strings.h>  	// Oh yeah!, bit functions that are probably hardwired to the ISA ops
#include <assert.h>

#define PASS_OK printf("\n[%d]\t%s",(++PASSES),"."); 
#define PASS_NOTOK printf("\n[%d]\t%s",(++PASSES),"FAIL");

int main() {

    int PASSES = 0;

	// =============================================
	// Are we able to create the underlying file 
	// that mimics a block storage device
	// =============================================
    assert(
	vbs_initialize("copyfile")
	);
// 1
PASS_OK

	// =============================================
	// This is the handle for the VBS
	// Lets try to open it
	// =============================================
    VBS_Type * testVBS = vbs_open("copyfile");
    assert(
	testVBS != NULL
	);
// 2
PASS_OK

	// =============================================
	// A block index variable
	// =============================================
    VBS_Index startBlockOfFile;
    VBS_Index idx = startBlockOfFile = vbs_nextFreeBlock(testVBS);

    int copiedBlocks = 0;

	// =============================================
	// Copy from file into VBS
	// =============================================
	// anon-code-block
	{
		int originalFile = open("../src/VBS_test.c", O_RDONLY);
		size_t readBytes = 0;
		if (originalFile != -1)  do
		{
			BlockType rwBlock = vbs_make_block();
			readBytes = read(originalFile, rwBlock.buffer, VBS_BLOCK_SIZE);
			vbs_write(testVBS, idx++, rwBlock);
			++copiedBlocks;
			vbs_release_block(rwBlock);
		} 
		while ( VBS_BLOCK_SIZE ==  readBytes );
		
		close(originalFile);
	}

	// =============================================
	// test close and re-open
	// =============================================

    assert(
	vbs_close(testVBS)
	);

// 5
PASS_OK

    testVBS = vbs_open("copyfile");

    assert(
	testVBS != NULL
	);
// 6
PASS_OK



	// =============================================
	// Copy from VBS into file
	// =============================================
	// anon-code-block
	{
		int newFile = open("copy_of_VBS_test.c", O_CREAT | O_WRONLY);
		idx = 8;
		idx = startBlockOfFile;
		if (newFile != -1)  do
		{
			BlockType rwBlock = vbs_read(testVBS, idx++);

			write(newFile, rwBlock.buffer, VBS_BLOCK_SIZE);
			
			vbs_release_block(rwBlock);
		} 
		while ( idx < startBlockOfFile + copiedBlocks );

		close(newFile);
	}


	FILE* pout = popen("diff ../src/VBS_test.c copy_of_VBS_test.c","r");
	if (pout == NULL)
	;    /* Handle error */;

char diffStr[512]; diffStr[511] = '\0';
printf("%s\n", "TESTING: Showing File Differences");
while (fgets(diffStr, 511, pout) != NULL)
    printf("%s", diffStr);


pclose(pout);

    assert(
	vbs_close(testVBS)
	);

// 7
PASS_OK


    remove("copyfile"); // comment this if you want to poke around the VBS file
    remove("copy_of_VBS_test.c");

    printf("\n %d Tests Passed!\n", PASSES);
    return 0;
}

