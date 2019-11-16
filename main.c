#include "sfs.h"
#include "disk.h"

#include "debug.h"

#include <string.h>
void disk_test();
void sfs_basic_test1();

int main(){
	// disk_test();
	
	sfs_basic_test1();
	return 0;
}


void sfs_basic_test1(){
	debug_func_init();
	char filename1[] = "file1";
	char filename2[] = "/file";
	int i1 = create_disk(filename1, BLOCKSIZE*100);
	if(i1 == 0){
		debug("Successfully created disk1\n");
	}	
	else{
		debug("Failed in creating disk1\n");
	}

	int fd1 = open_disk(filename1);

	int format_err = format(fd1);
	if(format_err<0){
		debug("Format error\n");
	}

	disk_stat* test ;
	test = get_disk_stat(fd1);
	debug("%d %d %d %d\n", test->blocks, test->reads, test->writes, test->size);
	free(test);
	debug_add_call_layer();
	int mount_err = mount(fd1);
	debug_remove_call_layer();
	if(mount_err<0){
		debug("Mount error\n");
	}else{
		debug("Successful mount\n");
	}
	debug_add_call_layer();
	int filec_err = create_file(filename2);
	debug_remove_call_layer();

	if(filec_err<0){
		debug("File create error\n");
	}else{
		debug("Successful mount\n");
	}	
	char str[4096];
	strcpy(str, "qwertyuiopasdfghjjklzxcvbnm");
	
	debug_add_call_layer();
	write_file(filename2,str, strlen(str), 0);
	debug_remove_call_layer();

	close_disk(fd1);

}	


void disk_test(){
	char* filename1 = "file1";
	char* filename2 = "file2";

	int i1 = create_disk(filename1, 4096*10);
	if(i1 == 0){
		debug("Successfully created disk1\n");
	}	
	else{
		debug("Failed in creating disk1\n");
	}

	int fd1 = open_disk(filename1);
	// printf(" fd is %d\n", fd1);
	// disk_stat* test = get_disk_stat(fd1);

	// printf("%d %d %d %d\n", test->blocks, test->reads, test->writes, test->size);
	// char str[4096];
	// strcpy(str, "qwertyuiopasdfghjjklzxcvbnm");
	// // free(test);
	// write_block(fd1,1, str);

	// test = get_disk_stat(fd1);

	// printf("%d %d %d %d\n", test->blocks, test->reads, test->writes, test->size);
	// // free(test);
	// char reader[4096];

	// read_block(fd1, 1, reader);
	

	// test = get_disk_stat(fd1);
	// printf("%d %d %d %d\n", test->blocks, test->reads, test->writes, test->size);
	// // free(test);
	// printf("%s\n", reader);
	// close_disk(fd1);
	// fd1 = open_disk(filename1);

	
	// read_block(fd1, 1, reader);
	

	// test = get_disk_stat(fd1);
	// printf("%d %d %d %d\n", test->blocks, test->reads, test->writes, test->size);
	// printf("%s\n", reader);
	// close(fd1);

	format(fd1);
	close_disk(fd1);
	
}
