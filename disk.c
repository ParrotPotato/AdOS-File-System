#include "disk.h"

#include <stdio.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>


int create_disk(char * filename, int nbytes)
{
	// creates a file with filename : filname 
	// and size : nbytes 
	
	int fd = open(filename, O_CREAT | O_WRONLY);
	int counter = 0;
	const char zero = 0;
	disk_stat status;

	if(fd < 0)
	{
		perror("Unable to create specified disk");
		return -1;
	}

	while(counter < nbytes)
	{
		write(fd, (void *)&zero, sizeof(zero));
		counter++;

	}

	lseek(fd, 0, SEEK_SET);

	status.size = nbytes;
	status.block = (nbytes - sizeof(disk_stat)) % BLOCK_SIZE;
	status.reads = 0;
	status.writes = 0;

	write(fd, (void *)&status, sizeof(status));

	close(fd);	

	return 0;
}
