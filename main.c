#include "sfs.h"
#include "disk.h"

void disk_test();

int main(){
	disk_test();
	return 0;
}

void disk_test(){
	char* filename1 = "file1";
	char* filename2 = "file2";

	int i1 = create_disk(filename1, 4096*10);
	if(i1 == 0){
		printf("Successfully created disk1\n");
	}	
	else{
		printf("Failed in creating disk1\n");
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