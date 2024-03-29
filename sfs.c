#include "sfs.h"


#include "disk.h"
#include <math.h>

#define MIN(x,y) (x) > (y) ? (y) : (x) 

int local_disk = -1;
super_block sb;
char* inode_bitmap;
char* data_bitmap;
int local_inode_num = -1;
int local_data_block_num = -1;

int init_inode(int inumber);
int init_root_directory();
int getFreeInode();

int nthMagicNo(int n) 
{ 
    int pow = 1, answer = 0; 
    while (n) 
    { 
       pow = pow*5; 
       if (n & 1) 
         answer += pow; 
  
       n >>= 1;
    } 
    return answer; 
} 
/**
 * Helper Functions
 */
int setbit(char* bits, int index)
{
    bits[index / (sizeof(char) * 8)] |= 1 << index % (sizeof(char) * 8);
    return 0;
}
// check for == 0 or not = 1 
int getbit(char* bits, int index)
{
    if((1 << index % (sizeof(char) * 8)) & (bits[index / (sizeof(char) * 8)])) return 1;
    return 0;
}
int unsetbit(char* bits, int index){

    bits[index / (sizeof(char) * 8)] &= ~(1 << index % (sizeof(char) * 8));
    return 0;
}


int checkMagicNumber(int mag){
    for(int i=1;;i++)
    {
        int t = nthMagicNo(i);
        if( t == i)
            return 1;
        if( t > i)
            return 0;
    }
}

inode load_inode(int inumber){
    int block_no = sb.inode_block_idx +  inumber/(BLOCKSIZE/sizeof(inode));
    int block_offset = inumber%(BLOCKSIZE/sizeof(inode));
    char block_reader[BLOCKSIZE];
    inode old_inode;
    if(read_block(local_disk, block_no, block_reader) < 0){
        perror("Inode Block read failed");
        return old_inode;
    }

    memcpy(&old_inode,block_reader + block_offset* sizeof(inode), sizeof(inode));
    return old_inode;
}

void store_inode(int inumber, inode inode_instance)
{
    int block_no = sb.inode_block_idx +  inumber/(BLOCKSIZE/sizeof(inode));
    int block_offset = inumber%(BLOCKSIZE/sizeof(inode));
    char block_reader[BLOCKSIZE];
    inode old_inode;
    if(read_block(local_disk, block_no, block_reader) < 0){
        perror("Inode Block read failed");
        return ;
    }

    memcpy(block_reader + block_offset * sizeof(inode), &inode_instance , sizeof(inode));
    write_block(local_disk, block_no, block_reader);
    return;
}

///////Functions to update the bitmaps///////////////////
int update_inode_bitmap_on_disk(super_block sb){

    int inode_bitmap_block_start = 1;
    int inode_bitmap_block_end  = sb.data_block_bitmap_idx;  

    char writer[BLOCKSIZE];

    for(int i=inode_bitmap_block_start;i<inode_bitmap_block_end;i++){
        memcpy(writer, inode_bitmap + (i-inode_bitmap_block_start)*BLOCKSIZE, BLOCKSIZE);
        if(write_block(local_disk, i, writer) < 0){
            perror("File create error, unable to update inode bitmap");
            return -1;
        }   
    }

    return 0;
}

int update_data_bitmap_on_disk(super_block sb){

    int data_bitmap_block_start = sb.data_block_bitmap_idx;
    int data_bitmap_block_end  = sb.inode_block_idx;  

    char writer[BLOCKSIZE];

    for(int i=data_bitmap_block_start;i<data_bitmap_block_end;i++){
        memcpy(writer, data_bitmap + (i-data_bitmap_block_start)*BLOCKSIZE, BLOCKSIZE);
        if(write_block(local_disk, i, writer) < 0){
            perror("File create error, unable to update inode bitmap");
            return -1;
        }   
    }

    return 0;
}

int free_bitmaps(super_block sb, inode node){
    int number_of_blocks = node.size%BLOCKSIZE == 0?node.size/BLOCKSIZE:node.size/BLOCKSIZE +1;

    for(int i = 0;i<5 && i<number_of_blocks;i++){
        unsetbit(data_bitmap, node.direct[i]);
    }
    if(number_of_blocks>5){
        number_of_blocks -= 5;
    }
    else{
        return update_inode_bitmap_on_disk(sb) && update_data_bitmap_on_disk(sb);
    }

    char rwer[BLOCKSIZE];

    //Read the indirect pointer array
    if(read_block(local_disk, node.indirect, rwer) < 0){
        perror("Unable to free inode in remove file");
        return -1;
    }

    for(int i=0;i<number_of_blocks;i++){
        int block_no;
        memcpy(&block_no, rwer + i*sizeof(int), sizeof(int));
        unsetbit(data_bitmap, block_no);
    }

    return update_inode_bitmap_on_disk(sb) && update_data_bitmap_on_disk(sb);
}

// Function to get the inode given inode number(@inumber)
inode getInode(int inumber){
    int block_no = sb.inode_block_idx +  inumber/(BLOCKSIZE/sizeof(inode));
    int block_offset = inumber%(BLOCKSIZE/sizeof(inode));

    inode read;
    read.valid = 0;
    char reader[BLOCKSIZE];
    if(read_block(local_disk, block_no, reader) < 0){
        printf("getInode: Read Failed\n");
        return read;
    }
    memcpy(&read, reader+ block_offset*sizeof(inode), sizeof(inode));
    
    return read;
}


//Find the file @name in the dir @inumber
int findFile(int inumber, char* name){
    

	inode dir_inode = getInode(inumber);
    int size = dir_inode.size;
    int number_of_files = size/sizeof(file_rep);
    int number_of_reps_in_block = BLOCKSIZE/sizeof(file_rep);
    int number_of_blocks = number_of_files/number_of_reps_in_block;

    
    char reader[BLOCKSIZE];
    for(int i=0;i<5 && i<number_of_blocks;i++){
        if(read_block(local_disk, dir_inode.direct[i], reader)<0){
            printf("FindFile - Block read error\n");
            return -1;
        }

        for(int j = 0;j<number_of_reps_in_block && j<(number_of_files - i*number_of_reps_in_block);j++){
            file_rep rep;
            memcpy(&rep, reader + j*sizeof(file_rep), sizeof(file_rep));
            if(strcmp(rep.name, name) == 0){
                return rep.inumber;
            }  
        }
    }
    
    number_of_blocks -= 5;
    
    if(number_of_blocks<=0){
        printf("findFile - Name not found in directory\n");
        return -1;
    }
    char inode_list[BLOCKSIZE];
    if(read_block(local_disk, dir_inode.indirect, inode_list)<0){
        printf("FindFile - Indirect Block read error\n");
        return -1;
    }

    for(int i = 0; i<BLOCKSIZE/sizeof(int) && i<number_of_blocks;i++){
        int blockno;
        memcpy(&blockno, inode_list + i*sizeof(int), sizeof(int));

        if(read_block(local_disk, blockno, reader)<0){
            printf("FindFile - Block read error\n");
            return -1;
        }

        for(int j = 0;j<number_of_reps_in_block && j<(number_of_files - (i+5)*number_of_reps_in_block);j++){
            file_rep rep;
            memcpy(&rep, reader + j*sizeof(file_rep), sizeof(file_rep));
            if(strcmp(rep.name, name) == 0){
                return rep.inumber;
            }  
        }
    }

    printf("findFile - File Not found\n");
    return -1;

}

int getInodeFromName(int inumber, char* token){
    int new_inode_num = findFile(inumber, token);// Find the file specified by token
    if(new_inode_num<0){
        printf("File not found while searching %s in inode %d\n", token, inumber);
        return -1;
    }
    token = strtok(NULL, "/"); // Get the next token
    if(token!= NULL){// If this is not final stage in the path 
        return getInodeFromName(new_inode_num, token);
    }
    
    //If this is the final stage, end recursion
    return new_inode_num;
}

//Create a file named @name in the directory with @inumber and @type
int initFile(int inumber, char* name, int type){
    /**
     * Find the details of the existing directory structure, so that reuse is possible. 
     */  
    inode dir_inode = getInode(inumber);
    int size = dir_inode.size;
    int number_of_files = size/sizeof(file_rep);
    int number_of_reps_in_block = BLOCKSIZE/sizeof(file_rep);
    int number_of_blocks = number_of_files/number_of_reps_in_block;
	
    
    /**
     * Start reading the blocks starting with the direct. If we find a file_rep that
     * is free, then we essentially  replace it with a new structure. 
     */ 
    char reader[BLOCKSIZE];
    int i=0;
    for(i=0;i<5 && i<number_of_blocks;i++){
        if(read_block(local_disk, dir_inode.direct[i], reader)<0){
            printf("initFile - Block read error\n");
            return -1;
        }

	printf("\n");
	printf("Read direct %d from local_disk\n", i);

        for(int j = 0;j<number_of_reps_in_block && j<(number_of_files - i*number_of_reps_in_block);j++){
            file_rep rep;
            memcpy(&rep, reader + j*sizeof(file_rep), sizeof(file_rep));
            if(rep.valid == 0){
                rep.type = type;
                rep.valid = 1;
                rep.name_size = strlen(name);
                strcpy(rep.name, name);
                rep.inumber = getFreeInode();
                memcpy(reader + j*sizeof(file_rep), &rep,  sizeof(file_rep));

                if(write_block(local_disk, dir_inode.direct[i], reader)<0){
                    printf("initFile - Block write error\n");
                    return -1;
                }
                else
                {
                    return init_inode(rep.inumber);
                }
                
            }  
        }
    }
    
    number_of_blocks -= 5;
    if(number_of_blocks>0){
        char inode_list[BLOCKSIZE];
        if(read_block(local_disk, dir_inode.indirect, inode_list)<0){
            printf("initFile - Indirect Block read error\n");
            return -1;
        }

        // Continue finding it in indirect block
        for(int i = 0; i<BLOCKSIZE/sizeof(int) && i<number_of_blocks;i++){
            int blockno;
            memcpy(&blockno, inode_list + i*sizeof(int), sizeof(int));

            if(read_block(local_disk, blockno, reader)<0){
                printf("initFile - Block read error\n");
                return -1;
            }

            for(int j = 0;j<number_of_reps_in_block && j<(number_of_files - (i+5)*number_of_reps_in_block);j++){
                file_rep rep;
                memcpy(&rep, reader + j*sizeof(file_rep), sizeof(file_rep));
                if(rep.valid == 0){
                    rep.type = type;
                    rep.valid = 1;
                    rep.name_size = strlen(name);
                    strcpy(rep.name, name);
                    rep.inumber = getFreeInode();
                    memcpy(reader + j*sizeof(file_rep), &rep,  sizeof(file_rep));
                    if(write_block(local_disk, blockno, reader)<0){
                        printf("initFile - Block write error\n");
                        return -1;
                    }
                    else{
                        return init_inode(rep.inumber);
                    }
                }   
            }
        }
    }
    /**
     * Write a new file_rep
     */
    file_rep rep;  
    rep.type = type;
    rep.valid = 1;
    rep.name_size = strlen(name);
    strcpy(rep.name, name);

    rep.inumber = getFreeInode();
    
    char* writer = (char*)malloc(sizeof(file_rep));
    memcpy(writer, &rep, sizeof(file_rep));
	
	
    if( write_i(inumber, writer, sizeof(file_rep),dir_inode.size) < 0){
        printf("initFile- Unable to write new file_rep\n");
        return -1;
    }


    int ret = init_inode(rep.inumber);
    return ret;
}

//Recursively find the directory where the new directory has to be created
int findDir(int inumber, char* token1, char* token2, int type){
    /**
     * If token 1 has to be written in current directory
     */ 
    if(token2 == NULL)
    {
        return initFile(inumber, token1,type);
    }

    // Get the inode of the  dir that we have to search/write token2 in
    int a =  getInodeFromName(inumber, token1);

    char token[MAXNAMESIZE];

    strcpy(token, token2);

    token2 = strtok(NULL, "/");

    return findDir(a, token, token2, type);
}


/////////////////////Library functions/////////////////////////////////
///////
int format(int disk){
    
    static int nth_magic_number = 1;

    disk_stat* stat = get_disk_stat(disk);

    int no_blocks = stat->blocks;
    int usable_blocks = no_blocks -1;    
    int num_inodes_blocks = 0.1*usable_blocks;
    int num_inodes = num_inodes_blocks*(BLOCKSIZE/sizeof(inode));
    double t1 = (double)((double)num_inodes/(8.0*4096.0));
    int t2 = t1;
    int num_inode_bitmap_blocks =  (t1 - (double)t2)>0.001?t2+1:t2;
    int remain_blocks = usable_blocks - num_inodes_blocks - num_inode_bitmap_blocks;
    t1 = (double)((double)remain_blocks/(8.0*4096.0));
    t2 = t1;
    int num_dbitmap_blocks = (t1 - (double)t2)>0.001?t2+1:t2;
    int num_data_blocks = remain_blocks - num_data_blocks;

    super_block sb;
    sb.magic_number = nthMagicNo(nth_magic_number++);
    sb.blocks = usable_blocks;
    sb.inode_blocks = num_inodes_blocks;
    sb.inodes = num_inodes;
    sb.inode_bitmap_block_idx = 1;
    sb.inode_block_idx = 1 + num_inode_bitmap_blocks + num_dbitmap_blocks;
    sb.data_block_bitmap_idx = 1 + num_inode_bitmap_blocks;
    sb.data_block_idx = 1 + num_inode_bitmap_blocks + num_dbitmap_blocks + num_inodes_blocks;
    sb.data_blocks = num_data_blocks;

    //Error Checking in the size of sb remaining

    char sb_writer[4096];
    memcpy(sb_writer, &sb, sizeof(super_block));
    char bitmap_block_set[4096];
    memset(bitmap_block_set, 0, BLOCKSIZE);

    if(write_block(disk, 0, sb_writer) < 0){
        perror("Format failed in writing super block");
        return -1;
    }

    for(int i = 1; i< sb.inode_block_idx; i++){
        if(write_block(disk, i, bitmap_block_set) < 0){
            perror("Format failed in initialiseing bitmaps");
            return -1;
        }   
    }

    inode base_inode;
    base_inode.valid = 0;
    char inode_set_block[4096];
    for(int i=0;i<(BLOCKSIZE/sizeof(inode));i++){
        memcpy(inode_set_block + i*sizeof(inode),&base_inode, sizeof(inode));
    }
    for( int i = sb.inode_block_idx; i< sb.data_block_idx ; i++){
        if(write_block(disk, i, inode_set_block) < 0){
            perror("Format failed in initialiseing inodes");
            return -1;
        } 
    }

    return 0;
}


int mount(int disk){
	char superBlockCopier[BLOCKSIZE];
    if(read_block(disk, 0, superBlockCopier) < 0){
        printf("Mount error, unable to mount super block\n");
        return -1;
    }

    memcpy(&sb, superBlockCopier, sizeof(super_block));
    if(checkMagicNumber(sb.magic_number)){
        printf("sb.magicNumber = %d\n", sb.magic_number);
        printf("Incorrect Magic Number during mount\n");
        return -1;
    }

    local_disk = disk;

    int number_of_inodeb_blocks = sb.data_block_bitmap_idx - 1 ;
    int number_of_datab_blocks = sb.inode_block_idx - sb.data_block_bitmap_idx;
    printf("No of databblocks %d\n",number_of_inodeb_blocks);
    inode_bitmap = (char*) malloc(number_of_inodeb_blocks*sizeof(char)*BLOCKSIZE);
    data_bitmap = (char*) malloc(number_of_datab_blocks*sizeof(char)*BLOCKSIZE);

    char bitmap_reader[BLOCKSIZE];
    memset(bitmap_reader, 0, BLOCKSIZE);

    for(int i=1;i<sb.data_block_bitmap_idx;i++){
        if(read_block(local_disk, i, bitmap_reader) < 0){
            perror("Initialising inode bitmaps during mount failed");
            return -1;
        }
        memcpy(inode_bitmap + (i-1)*BLOCKSIZE, bitmap_reader, BLOCKSIZE);
    }
    
    for(int i=sb.data_block_bitmap_idx;i<sb.inode_block_idx;i++){
        if(read_block(local_disk, i, bitmap_reader) < 0){
            perror("Initialising data bitmaps during mount failed");
            return -1;
        }
        memcpy(data_bitmap + (i-sb.data_block_bitmap_idx)*BLOCKSIZE, bitmap_reader, BLOCKSIZE);
    }

    local_inode_num = sb.inodes;
    local_data_block_num = sb.data_blocks;
    
    return init_root_directory();
}

int create_file(char* filename1){
	int inumber = 0;
    char filename[MAXNAMESIZE];
    strcpy(filename, filename1);
    
    char* token1 = strtok(filename, "/");
    
    char* token2 = strtok(NULL, "/");
	
    
    

    if(token2 == NULL){
	int ret = initFile(inumber, token1, 1);
        return ret;
    }
    else{
        return findDir(inumber, token1, token2, 1);
    }
}

int remove_file(int inumber){
    
    char block_reader[BLOCKSIZE];
    int block_no = sb.inode_block_idx +  inumber/(BLOCKSIZE/sizeof(inode));
    int block_offset = inumber%(BLOCKSIZE/sizeof(inode));
    inode old_inode;

    if(read_block(local_disk, block_no, block_reader) < 0){
        perror("Remove file, inode read failed");
        return -1;
    }
    old_inode.valid = 0;
    unsetbit(inode_bitmap, inumber);
    int r = free_bitmaps(sb, old_inode);
    
    if(r!=0){
        perror("Unable to clear bitmaps in file remove");
        return -1;
    }
    
    memcpy(block_reader + block_offset*sizeof(inode), &old_inode, sizeof(inode));
    if(write_block(local_disk, block_no, block_reader) < 0){
        perror("File remove error, unable to write inode");
        return -1;
    }
    return 0;
}

inode stat(int inumber){
    inode old_inode;
    old_inode.valid = 0;

    int block_no = sb.inode_block_idx +  inumber/(BLOCKSIZE/sizeof(inode));
    char block_reader[BLOCKSIZE];
    if(read_block(local_disk, block_no, block_reader) < 0){
        perror("File create error, unable to mount read block");
        return old_inode;
    }

    
    int block_offset = inumber%(BLOCKSIZE/sizeof(inode));
    memcpy(&old_inode,block_reader + block_offset*sizeof(inode), sizeof(inode));

    return old_inode;
}

// Local function for getting block from given offset 
//

static int get_indirect_block(inode fileinode, int blockindex)
{
	int i=0;

	char readbuffer[BLOCKSIZE];
	
	read_block(local_disk, fileinode.indirect, readbuffer);

	return * (int  *) (readbuffer + 4 * blockindex);
}


static int get_indirect_block_index_by_block_no(inode fileinode, int blocknumber)
{
	const int maxentries = 1024;

	int i = 0;

	for(i = 0; i < maxentries ; i++)
	{
		if(get_indirect_block(fileinode, i) == blocknumber) return i;
	}

	return -1;
}

// TODO: Error checking for offset being more than the amount that 
// can be stored in the reading data blocik -nitesh
//
static int get_block_for_offset(int offset, inode fileinode)
{
	// if offset in first 5 blocks then return one with the need 
	if(offset < 5 * BLOCKSIZE)
		return fileinode.direct[offset / BLOCKSIZE];
	
	int off_directblocks = offset - 5 * BLOCKSIZE;

	int index_in_indirect_block_pointer = off_directblocks/ BLOCKSIZE;
	
	return get_indirect_block(fileinode, index_in_indirect_block_pointer);
}

enum
{
	DIRECT,
	INDIRECT
};


// returns DIRECT is the block is in the direct list of the inode 
// else reutrn INDIRECT 
static int get_block_type(inode fileinode, int block)
{
	int i=0 ;

	for(i = 0 ; i < 5  ; i++) if(block == fileinode.direct[i]) return DIRECT;
	else return INDIRECT;
}

// 
// API call for reading from a file given its inumber, offset and length
// the function reads the disk block by block and updates the data in the 
// data buffer provided.
//

int read_i(int inumber, char *data, int length, int offset){
    if(getbit(inode_bitmap, inumber) == 0){
        printf("Incorrect inode number in read_i\n");
        return -1;
    }
    inode old_inode = load_inode(inumber);
    if(offset>old_inode.size){
        printf("Invalid offset\n");
        return -1;
    }

    int read_len = MIN(length, old_inode.size - offset);
    int number_of_blocks, start_block = offset/BLOCKSIZE,start_of = offset%BLOCKSIZE;
    

	int currentblock = get_block_for_offset(offset, old_inode);
	int type = get_block_type(old_inode, currentblock);
	int directcounter = -1;
	int indirectcounter = -1;
	
	// setting the counter for direct block

	if(type == DIRECT)
	{
		for(; directcounter < 5 ; directcounter++)
			if(old_inode.direct[directcounter] == currentblock) break;
		if(directcounter == 5) directcounter = -1;
	}
	else 
	{
		indirectcounter = get_indirect_block_index_by_block_no(old_inode, currentblock);
	}

	char readingbuffer[BLOCKSIZE];
	int  readingoffset = 0;
	while(read_len > 0)
	{
		// Reading from disk 
		read_block(local_disk, currentblock, readingbuffer);
		
		// putting data in buffer and updating the read count and
		// remaining read count 

		// for first iteration where we need to start copying from 
		// some intermion position instead of the beginning of the 
		// buffer 		
		if(start_of != 0)
		{
			if(start_of + read_len > BLOCKSIZE)
			{
				memcpy(data + readingoffset, readingbuffer + start_of, BLOCKSIZE - start_of);
				readingoffset += BLOCKSIZE - start_of;
				read_len -= BLOCKSIZE - start_of;
				start_of = 0;
			}
			else 
			{
				memcpy(data + readingoffset, readingbuffer + start_of, read_len);
				readingoffset += read_len;
				read_len = 0; 
				start_of = 0;
				break;
			}
		}
		
		// here we will be copying the buffer from the beginning 
		else 
		{
			if(read_len < BLOCKSIZE)
			{
				memcpy(data + readingoffset, readingbuffer, read_len);
				readingoffset += read_len;
				read_len = 0;
				break;
			}
			else 
			{
				memcpy(data + readingoffset, readingbuffer, BLOCKSIZE);
				readingoffset += BLOCKSIZE;
				read_len -= BLOCKSIZE;
			}
		}

		// updating the memory block for next read
		// if the current block was in the direct block list of inode
		//
		// 		
		if(directcounter != -1)
		{
			directcounter += 1;
			if(directcounter == 5)
			{
				directcounter = -1;
				// get first indirect block 
				
				int temp = 0;
				currentblock = get_indirect_block(old_inode, 0);
				indirectcounter = 0;
			}
			else 
			{
				currentblock = old_inode.direct[directcounter];
			}
		}
		// if the current block was in the indirect block list of inode
		else 
		{
			currentblock = get_indirect_block(old_inode, indirectcounter + 1);
			indirectcounter += 1;
		}

	}

	return readingoffset;
}


// Functions for helping the callers


int set_data_block(int i)
{
	setbit(data_bitmap, i);
}

int unset_data_block(int i)
{
	unsetbit(data_bitmap, i);
}

// function to return empty data blocks 
int get_empty_data_block()
{
	unsigned int maxcount = 8 * BLOCKSIZE * (sb.inode_block_idx - sb.data_block_bitmap_idx);
	
	unsigned int i = 0 ;

	for(i = 0; i < maxcount ; i++)
	{
		if(getbit(data_bitmap, i) == 0) return i;
	}

	return -1;
}


// function allocated required number of data blocks given the 
// filenode. It returns the number of blocks allocated and if the 
// specifiec number of blocks are not avaliable then returns the 
// number of blocks that at max call be allocated.
int allocate_data_blocks(inode  *fileinode, int blocks)
{

	if(blocks == 0) return 0;
	
	int currentlyallocated = fileinode->size / BLOCKSIZE;
	int newallocated= 0;

	if(currentlyallocated <5)
	{
		while(currentlyallocated < 5 && newallocated < blocks)
		{
			int temp = get_empty_data_block();
			if(temp == -1) return newallocated;
			
			fileinode->direct[currentlyallocated] = temp;
			currentlyallocated += 1;
			newallocated += 1;

			set_data_block(temp);
		}
		
		if(newallocated == blocks) return newallocated;
	}

	currentlyallocated = currentlyallocated - 5 ;

	char readingbuffer[BLOCKSIZE];
	int indirectallocations = 0 ;
	
	// indirect block needs to be allocated to the file
	if(currentlyallocated == 0)
	{
		int temp = get_empty_data_block();
		if(temp == -1) return newallocated;

		fileinode->indirect = temp;

		set_data_block(temp);
	}

	read_block(local_disk, fileinode->indirect, readingbuffer);

	while(newallocated < blocks && indirectallocations < 1024)
	{
		int temp = get_empty_data_block();
		if(temp = -1){
			write_block(local_disk, fileinode->indirect, readingbuffer);
			return newallocated;
		} 
		
		memcpy(readingbuffer + 4 * currentlyallocated, &temp, 4);

		currentlyallocated += 1;
		newallocated += 1;
		indirectallocations += 1;

		set_data_block(temp);
	}

	write_block(local_disk, fileinode->indirect, readingbuffer);

	update_data_bitmap_on_disk(sb);

	return newallocated;
}

//
// API interface for write call. Takes the inumbde, buffer, length
// and offset set and writes to the specified file. (specified via
// inumber). Wirte call allocated required blocks on the go and if
// required blocks are not avaliable then just writes as much as 
// it can. 

int write_i(int inumber, char *data, int length, int offset)
{
	if(getbit(inode_bitmap, inumber) == 0)
	{
		printf("Incorrect Inumber\n");
		return -1;
	}
	inode old_inode = load_inode(inumber);
	if(offset > old_inode.size)
	{
		printf("Incorrect offset \n");
		return -1;
	}

	int bytesrequired = (offset + length - old_inode.size);
	int blocksrequired = bytesrequired % BLOCKSIZE == 0 ? bytesrequired / BLOCKSIZE : bytesrequired / BLOCKSIZE + 1;

	int write_len = MIN(allocate_data_blocks(&old_inode, blocksrequired) * 8, length);
	int initialoffset = offset % BLOCKSIZE;

	if(write_len == 0) 
	{
		printf("Write len calculated to be 0 : Returning \n");
		return 0;
	}

	int currentblock = get_block_for_offset(offset, old_inode);
	int type = get_block_type(old_inode, currentblock);
	int directcounter = -1;
	int indirectcounter = -1;
	
	// setting the counter for direct block
	

	if(type == DIRECT)
	{
		for(; directcounter < 5 ; directcounter++)
			if(old_inode.direct[directcounter] == currentblock) break;
		if(directcounter == 5) directcounter = -1;
	}
	else 
	{
		indirectcounter = get_indirect_block_index_by_block_no(old_inode, currentblock);
	}

	

	char writingbuffer[BLOCKSIZE];
	int writingoffset = 0;
	int size = old_inode.size;

	while(write_len > 0)
	{
		// reading the file buffer
		read_block(local_disk, currentblock, writingbuffer);


		// modifying the file buffer
		if(initialoffset != 0 )
		{
			if(initialoffset + write_len > BLOCKSIZE)
			{
				memcpy(writingbuffer + initialoffset, data + writingoffset, BLOCKSIZE - initialoffset);
				write_block(local_disk, currentblock, writingbuffer);

				writingoffset += BLOCKSIZE - initialoffset;
				write_len -= BLOCKSIZE - initialoffset;
				initialoffset = 0;
				size += BLOCKSIZE - initialoffset;
			}
			else 
			{
				memcpy(writingbuffer + initialoffset, data + writingoffset, write_len);
				write_block(local_disk, currentblock, writingbuffer);

				writingoffset += write_len;
				write_len = 0;
				initialoffset = 0;
				size += write_len;
				
				break;
			}
		}

		else 
		{
			if(write_len < BLOCKSIZE)
			{
				memcpy(writingbuffer, data + writingoffset, write_len);
				write_block(local_disk, currentblock, writingbuffer);

				writingoffset  += write_len;
				write_len  = 0;
				size += write_len;
				break;
			}
			else 
			{
				memcpy(writingbuffer, data + writingoffset, BLOCKSIZE);
				write_block(local_disk, currentblock, writingbuffer);

				writingoffset  += BLOCKSIZE;
				write_len  -= BLOCKSIZE;
				size += BLOCKSIZE;
				break;
			}
		}


		// updating to next block
		
		if(directcounter != -1)
		{
			directcounter += 1;
			if(directcounter == 5)
			{
				directcounter = -1;
				// get first indirect block 
				
				int temp = 0;
				currentblock = get_indirect_block(old_inode, 0);
				indirectcounter = 0;
			}
			else 
			{
				currentblock = old_inode.direct[directcounter];
			}
		}
		// if the current block was in the indirect block list of inode
		else 
		{
			currentblock = get_indirect_block(old_inode, indirectcounter + 1);
			indirectcounter += 1;
		}
	}
	
	old_inode.size = size;
	// restoring with the new file size 
	store_inode(inumber, old_inode);

	return writingoffset;
}

// 
// Function call for reading directly from the given file path.
// It internally makes a call to read_i 
//

int read_file(char *filepath, char *data, int length, int offset){
    char* token = strtok(filepath, "/");

    if(token == NULL)
    {
        printf("Incorrect file name\n");
        return -1;
    }
    //Recursively decide the inode number of the file to read 
    int inode_num = getInodeFromName(0, token);
    
    if(inode_num<0){
        printf("Retirval of inode_num from dir failed\n");
        return -1;
    }

    return read_i(inode_num, data, length, offset);
}

// 
// Function call for writing directly to the given file path.
// It internally makes a call to write_i  
//

int write_file(char *filepath, char *data, int length, int offset){
	char* token = strtok(filepath, "/");
    
    if(token == NULL)
    {
        printf("Incorrect file name\n");
        return -1;
    }
    //Recursively decide the inode number of the file to read 
    int inode_num = getInodeFromName(0, token);
    
    if(inode_num<0){
        printf("Retirval of inode_num from dir failed\n");
        return -1;
    }
	int ret = write_i(inode_num, data, length, offset);
    return ret;
}

//Initialise the inode @inumber
int init_inode(int inumber){
	/**
     * Set the inode bitmap
     */
    setbit(inode_bitmap, inumber);
    
    int block_no = sb.inode_block_idx +  inumber/(BLOCKSIZE/sizeof(inode));
    int block_offset = inumber%(BLOCKSIZE/sizeof(inode));
	

    char reader[BLOCKSIZE];
    if(read_block(local_disk, block_no, reader) < 0){
        printf("init_inode: Read Failed\n");
        return -1;
    }
    inode read;
    read.valid = 1;
    read.size = 0;
    read.indirect = -1;
    for(int i=0;i<5;i++)
        read.direct[i] = -1;
	

    memcpy(reader+ block_offset*sizeof(inode), &read, sizeof(inode));

    if(write_block(local_disk, block_no, reader) < 0){
        printf("init_inode: Write Failed\n");
        return -1;
    }
    return update_inode_bitmap_on_disk(sb);
}

//
// Function to get the free inode from the avaliable 
// inodes on the disk.
//

int getFreeInode(){
    int i;
    for(i=0;i<local_inode_num;i++)
    {
        if(getbit(inode_bitmap, i) == 0){
            break;   
        }
    }

    if( i >= local_inode_num){
        printf("No inode left to create file\n");
        return -1;
    }   
   	return i;
}

// 
// initializing root directory with inode number 
// 0

int init_root_directory(){
    init_inode(0);
}


// Function creates a directory with the given 
// path.
//
int create_dir(char *dirpath){
    int inumber = 0;
    if(strcmp(dirpath, "/") == 0){
       return init_root_directory();
    }       

    char* token1 = strtok(dirpath, "/");
    
    char* token2 = strtok(NULL, "/");

    if(token2 == NULL)
    {
        return initFile(inumber,token1, 0);
    }
    else{
        return findDir(inumber, token1, token2, 0);
    }

}

int clear_dir(int inumber){

}

int findRemoveFile(int inumber, char* name){
    inode dir_inode = getInode(inumber);
    int size = dir_inode.size;
    int number_of_files = size/sizeof(file_rep);
    int number_of_reps_in_block = BLOCKSIZE/sizeof(file_rep);
    int number_of_blocks = number_of_files/number_of_reps_in_block;

    char reader[BLOCKSIZE];
    for(int i=0;i<5 && i<number_of_blocks;i++){
        if(read_block(local_disk, dir_inode.direct[i], reader)<0){
            printf("FindRemoveFile - Block read error\n");
            return -1;
        }

        for(int j = 0;j<number_of_reps_in_block && j<(number_of_files - i*number_of_reps_in_block);j++){
            file_rep rep;
            memcpy(&rep, reader + j*sizeof(file_rep), sizeof(file_rep));
            if(strcmp(rep.name, name) == 0){
                rep.valid = 0;
                memcpy(reader + j*sizeof(file_rep), &rep, sizeof(file_rep));
                if(write_block(local_disk, dir_inode.direct[i], reader)<0){
                    printf("FindRemoveFile - Block write error\n");
                    return -1;
                }
            }  
        }
    }
    
    number_of_blocks -= 5;
    
    if(number_of_blocks<=0){
        printf("findFile - Name not found in directory\n");
        return -1;
    }
    char inode_list[BLOCKSIZE];
    if(read_block(local_disk, dir_inode.indirect, inode_list)<0){
        printf("FindFile - Indirect Block read error\n");
        return -1;
    }

    for(int i = 0; i<BLOCKSIZE/sizeof(int) && i<number_of_blocks;i++){
        int blockno;
        memcpy(&blockno, inode_list + i*sizeof(int), sizeof(int));

        if(read_block(local_disk, blockno, reader)<0){
            printf("FindRemoveFile - Block read error\n");
            return -1;
        }

        for(int j = 0;j<number_of_reps_in_block && j<(number_of_files - (i+5)*number_of_reps_in_block);j++){
            file_rep rep;
            memcpy(&rep, reader + j*sizeof(file_rep), sizeof(file_rep));
            if(strcmp(rep.name, name) == 0){
                rep.valid = 0;
                memcpy(reader + j*sizeof(file_rep), &rep, sizeof(file_rep));
                if(write_block(local_disk,blockno, reader)<0){
                    printf("FindRemoveFile - Block write error\n");
                    return -1;
                }
            }  
        }
    }

    printf("findFile - File Not found\n");
    return -1;

}

int remove_dir(char* dirpath){
    if(strcmp(dirpath, "/") == 0){
       printf("Can't remove root dir\n");
       return -1;
    }       
    int inumber = getInodeFromName(0, dirpath);
    
    inode dir_inode = getInode(inumber);
    int size = dir_inode.size;
    int number_of_files = size/sizeof(file_rep);
    int number_of_reps_in_block = BLOCKSIZE/sizeof(file_rep);
    int number_of_blocks = number_of_files/number_of_reps_in_block;

    char reader[BLOCKSIZE];
    for(int i=0;i<5 && i<number_of_blocks;i++){
        if(read_block(local_disk, dir_inode.direct[i], reader)<0){
            printf("FindFile - Block read error\n");
            return -1;
        }

        for(int j = 0;j<number_of_reps_in_block && j<(number_of_files - i*number_of_reps_in_block);j++){
            file_rep rep;
            memcpy(&rep, reader + j*sizeof(file_rep), sizeof(file_rep));
            if( rep.valid == 1){
                if(rep.type == 1)
                    remove_file(rep.inumber);
                else
                {
                    remove_dir(rep.name);
                }
            }  
        }
    }
    
    number_of_blocks -= 5;
    
    if(number_of_blocks<=0){
        printf("remove_dir - Name not found in directory\n");
        return -1;
    }

    char inode_list[BLOCKSIZE];
    if(read_block(local_disk, dir_inode.indirect, inode_list)<0){
        printf("remove_dir - Indirect Block read error\n");
        return -1;
    }

    for(int i = 0; i<BLOCKSIZE/sizeof(int) && i<number_of_blocks;i++){
        int blockno;
        memcpy(&blockno, inode_list + i*sizeof(int), sizeof(int));

        if(read_block(local_disk, blockno, reader)<0){
            printf("FindFile - Block read error\n");
            return -1;
        }

        for(int j = 0;j<number_of_reps_in_block && j<(number_of_files - (i+5)*number_of_reps_in_block);j++){
            file_rep rep;
            memcpy(&rep, reader + j*sizeof(file_rep), sizeof(file_rep));
            if( rep.valid == 1){
                if(rep.type == 1)
                    remove_file(rep.inumber);
                else
                {
                    remove_dir(rep.name);
                }
            }    
        }
    }

    int inumber_prev = inumber;
    char* token1 = strtok(dirpath, "/");
    inumber = findFile(inumber_prev, token1);
    char* token2 = strtok(NULL, "/");
    
    while (token2 != NULL)
    {
        token1 = token2;
        token2 = strtok(NULL, "/");
        inumber_prev = inumber;
        inumber = findFile(inumber_prev, token1);
    }

    return findRemoveFile(inumber_prev, token1);
}
