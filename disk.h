#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>


const static int BLOCKSIZE = 4*1024;


typedef struct disk {
	uint32_t size; // size of the entire disk file
	uint32_t blocks; // number of blocks (except stat block)
	uint32_t reads; // number of block reads performed
	uint32_t writes; // number of block writes performed
} disk_stat;


int create_disk(char *filename, int nbytes);

int open_disk(char *filename);

disk_stat* get_disk_stat(int disk);

int read_block(int disk, int blocknr, void *block_data);

int write_block(int disk, int blocknr, void *block_data);

int close_disk(int disk);
