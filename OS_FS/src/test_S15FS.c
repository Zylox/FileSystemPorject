/*
 * One our official grading example test programs for usage for grading
 * for your S15 File System.
 * */
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "../include/S15Filesystem.h"

#define PASS_OK printf("[%d]\t%s\n",(++PASSES),"."); 
#define PASS_NOTOK printf("[%d]\t%s\n",(++PASSES),"FAIL");


#define TEST_DATA_SIZE 10240 // 10KB

void createRandomData(unsigned int *data, unsigned int size) {
	int i = 0;
	for (;i < size; ++i) {
		data[i] = rand() % 100 + 1;
	}
}

int main (int argc, char **argv) {
	
	srand(time(NULL));
	unsigned int test_data[TEST_DATA_SIZE] = {0};
	unsigned int cmp_test_data[TEST_DATA_SIZE] = {0};
	createRandomData(test_data,TEST_DATA_SIZE);
	memcpy(&cmp_test_data, &test_data,TEST_DATA_SIZE);
	
	int PASSES = 0;

	assert (fs_mount() >= 0);
	assert (fs_create_file("/testDir", DIR_FILE) >= 0);
	assert (fs_unmount() >= 0);

PASS_OK

	assert (fs_mount() >= 0);
	assert (fs_create_file("/testDir", DIR_FILE) >= 0);
	assert (fs_create_file("/testDir/checkMe.txt", REG_FILE) >= 0);
	assert (fs_unmount() >= 0);
	
PASS_OK


	assert (fs_mount() >= 0);
	assert (fs_create_file("/testDir", DIR_FILE) >= 0);
	assert (fs_create_file("/testDir/checkMe.txt", REG_FILE) >= 0);
	assert (fs_write_file("/testDir/checkMe.txt", test_data, TEST_DATA_SIZE) >= 0);
	assert (fs_unmount() >= 0);
	
PASS_OK

	unsigned int readResults[TEST_DATA_SIZE] = {0};
	
	assert (fs_mount() >= 0);
	assert (fs_create_file("/testDir", DIR_FILE) >= 0);
	assert (fs_create_file("/testDir/checkMe.txt", REG_FILE) >= 0);
	assert (fs_write_file("/testDir/checkMe.txt", test_data, TEST_DATA_SIZE) >= 0);
	assert (fs_read_file("/testDir/checkMe.txt", readResults, TEST_DATA_SIZE) >= 0);
	int i = 0;
	for (;i < TEST_DATA_SIZE; ++i) {
		assert(readResults[i] == cmp_test_data[i]);
	}
	assert (fs_unmount() >= 0);
	
PASS_OK
	
	assert (fs_mount() >= 0);
	assert (fs_create_file("/testDir", DIR_FILE) >= 0);
	assert (fs_create_file("/testDir/checkMe.txt", REG_FILE) >= 0);
	assert (fs_create_file("/testDir/secondMe.txt", REG_FILE) >= 0);
	assert (fs_create_file("/testDir/thirdMe.txt", REG_FILE) >= 0);
	
	Directory_t dir;
	assert (fs_get_directory("/testDir/", &dir) >= 0);
	assert (dir.size == 3);
	assert(strncmp(dir.entries[0].filename,"checkMe.txt",strlen("checkMe.txt")) == 0);
	assert(strncmp(dir.entries[1].filename,"secondMe.txt",strlen("secondMe.txt")) == 0);
	assert(strncmp(dir.entries[2].filename,"thirdMe.txt",strlen("thirdMe.txt")) == 0);
	assert (fs_unmount() >= 0);
	
PASS_OK

	return 0;
}
