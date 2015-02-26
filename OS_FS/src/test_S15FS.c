/*
 * Sample example test program for usage for writing test
 * for your S15 File System.
 * */
#include <assert.h>
#include <stdio.h>

#include "../include/S15Filesystem.h"

#define PASS_OK printf("[%d]\t%s\n",(++PASSES),"."); 
#define PASS_NOTOK printf("[%d]\t%s\n",(++PASSES),"FAIL");


int main (int argc, char **argv) {
	
	int PASSES = 0;

	assert (fs_mount());
		
	assert (fs_unmount());

PASS_OK

	fs_mount();
	fs_create_file("/bob/", DIR_FILE);
	fs_unmount();

PASS_OK

	return 0;
}
