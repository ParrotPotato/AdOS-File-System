#include "disk.h"

//////////////////////////////////////////////////////////////////
/* An array to map the disk_stat structure to the file descriptor*/
disk_stat* fd_2_disk_stat[512];
/////////////////////////////////////////////////////////////////

int create_disk(char *filename, int nbytes){
    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
    
    disk_stat d;
    
    d.blocks = (int)(nbytes - sizeof(disk_stat))/4096;
    d.reads = 0;
    d.writes = 0;
    d.size = nbytes;
    
    int err = write(fd, &d, sizeof(disk_stat));
    if(err < 0){
        perror("Error in creating disk");
        close(fd);
        return -1;
    }
    int stretch = lseek(fd, nbytes-1, SEEK_SET);
    if(stretch<0){
        perror("Failed to create file");
        return -1;
    }
    close(fd);
    return 0;
}

int open_disk(char *filename){
    int fd = open(filename, O_RDWR, S_IRWXU);
    if(fd<0){
        perror("Error opening disk");
        return -1;
    }
    disk_stat* fd_disk_stat = (disk_stat*)malloc(sizeof(disk_stat));

    read(fd, fd_disk_stat, sizeof(disk_stat));
    fd_2_disk_stat[fd] = fd_disk_stat;
    
    return fd;
}

disk_stat* get_disk_stat(int disk){
    return fd_2_disk_stat[disk];
}

int read_block(int disk, int blocknr, void *block_data){
    int number_of_blocks = fd_2_disk_stat[disk]->blocks;
    if(blocknr >= number_of_blocks ||  blocknr <0){
        perror("Read - Incorrect Block numebr");
        return -1;
    }
    off_t offset_in = sizeof(disk_stat) + blocknr*4096;
    off_t offset_out = lseek(disk, offset_in, SEEK_SET); 

    if(offset_in != offset_out)
    {
        perror("Read- Offset reset error");
        return -1;
    }

    int read_err = read(disk, block_data, 4096);
    
    fd_2_disk_stat[disk]->reads++;

    offset_out = lseek(disk, 0, SEEK_SET); 
    if(write(disk, fd_2_disk_stat[disk], sizeof(disk_stat))<0){
        perror("Read - Stat Update error");
        return -1;
    }

    if(read_err<0){
        perror("Disk Read error");
        return -1;
    }

    return 0;
}

int write_block(int disk, int blocknr, void *block_data){
    int number_of_blocks = fd_2_disk_stat[disk]->blocks;
    if(blocknr >= number_of_blocks ||  blocknr <0){
        perror("Write - Incorrect Block numebr");
        return -1;
    }

    off_t offset_in = sizeof(disk_stat) + blocknr*4096;
    off_t offset_out = lseek(disk, offset_in, SEEK_SET); 
    if(offset_in != offset_out)
    {
        perror("Write - Offset reset error");
        return -1;
    }

    int write_err = write(disk, block_data, 4096);

    if(write_err<0){
        perror("Disk Write error");
        return -1;   
    }

    fd_2_disk_stat[disk]->writes++;

    offset_out = lseek(disk, 0, SEEK_SET); 
    if(write(disk, fd_2_disk_stat[disk], sizeof(disk_stat))<0){
        perror("Write - Stat Update error");
        return -1;
    }

    return 0;
}

int close_disk(int disk){
    free(fd_2_disk_stat[disk]);
    close(disk);
    return 0;
}