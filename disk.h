#ifndef HEADER_DISK
#define HEADER_DISK

#define BLOCK_SIZE 4019

#include <stdint.h>

typedef struct disk{
	uint32_t 	size;
	uint32_t 	block;
	uint32_t 	reads;
	uint32_t	writes;
} disk_stat;

int create_disk(char * filename, int nbytes);

int open_disk(char * filename, int nbytes);

disk_stat * get_disk_stat(int disk);

int read_block(int disk, int blocknr, void * block_data);

int write_block(int disk, int blocknr, void * block_data);

int close_disk(int disk);

#endif
