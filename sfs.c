#include "sfs.h"
#include "disk.h"
#include <math.h>


int local_disk = -1;
super_block sb;
char *inode_bitmap;
char* data_bitmap;
int local_inode_num = -1;
int local_data_block_num = -1;


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
    int block_no = sb.inode_block_idx +  inumber/128;
    int block_offset = inumber%128;
    char block_reader[BLOCKSIZE];
    inode old_inode;
    if(read_block(local_disk, block_no, block_reader) < 0){
        perror("Inode Block read failed");
        return old_inode;
    }

    memcpy(&old_inode,block_reader + block_offset*sizeof(inode), sizeof(inode));
    return old_inode;
}

///////Functions to update the bitmaps///////////////////
int update_inode_bitmap(super_block sb){

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

int update_data_bitmap(super_block sb){

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
        return update_inode_bitmap(sb) && update_data_bitmap(sb);
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

    return update_inode_bitmap(sb) && update_data_bitmap(sb);
}


/////////////////////Library functions/////////////////////////////////

int format(int disk){
    
    static int nth_magic_number = 1;

    disk_stat* stat = get_disk_stat(disk);

    int no_blocks = stat->blocks;
    int usable_blocks = no_blocks -1;    
    int num_inodes_blocks = 0.1*usable_blocks;
    int num_inodes = num_inodes_blocks*128;
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
    for(int i=0;i<128;i++){
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
        perror("Mount error, unable to mount super block");
        return -1;
    }

    memcpy(&sb, superBlockCopier, sizeof(super_block));
    if(!checkMagicNumber(sb.magic_number)){
        perror("Incorrect Magic Number during mount");
        return -1;
    }

    local_disk = disk;

    int number_of_inodeb_blocks = sb.data_block_bitmap_idx - 1 ;
    int number_of_datab_blocks = sb.inode_block_idx - sb.data_block_bitmap_idx;
    
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

    return 0;
}

int create_file(){
    int i;
    for(i=0;i<local_inode_num;i += 8)
    {
        if(getbit(inode_bitmap, i) == 0){
            setbit(inode_bitmap, i);
            break;   
        }
    }

    if( i >= local_inode_num){
        printf("No inode left to create file\n");
        return -1;
    }

    inode new_inode;
    new_inode.size = 0;
    new_inode.valid = 1;
    for(int i=0;i<5;i++){
        new_inode.direct[i] = -1;
    }
    new_inode.indirect = -1;

    int block_no = sb.inode_block_idx +  i/128;
    char block_reader[BLOCKSIZE];
    if(read_block(local_disk, block_no, block_reader) < 0){
        perror("File create error, unable to mount read block");
        return -1;
    }

    int block_offset = i%128;
    memcpy(block_reader + block_offset*sizeof(inode), &new_inode, sizeof(inode));

    if(write_block(local_disk, block_no, block_reader) < 0){
        perror("File create error, unable to write inode");
        return -1;
    }

    return update_inode_bitmap(sb);
}


int remove_file(int inumber){
    
    char block_reader[BLOCKSIZE];
    int block_no = sb.inode_block_idx +  inumber/128;
    int block_offset = inumber%128;
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

    int block_no = sb.inode_block_idx +  inumber/128;
    char block_reader[BLOCKSIZE];
    if(read_block(local_disk, block_no, block_reader) < 0){
        perror("File create error, unable to mount read block");
        return old_inode;
    }

    
    int block_offset = inumber%128;
    memcpy(&old_inode,block_reader + block_offset*sizeof(inode), sizeof(inode));

    return old_inode;
}

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

    int read_len = min(length, old_inode.size - offset);

    int number_of_blocks, start_block = offset/BLOCKSIZE,start_of = offset%BLOCKSIZE;

    
}