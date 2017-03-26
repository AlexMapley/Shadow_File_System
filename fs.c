#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "disk_emu.h"



#define BLOCK_SIZE 1024;
#define MAX_BLOCK 1024;
#define DISK_NAME "MapleDisk"




/********************************************************************************************************************/
/* STRUCTURES IN SYSTEM
Designates the information and design
principles needed for each component in our
shadow file system. At the very least,
this can be used as a reference to 
malloc'd size */


/*************************/
//File
typedef struct file {

	char data[4096];
	
} file;




/*************************/
//Inode
typedef struct inode {

	//Size in bytes = 16 + 4 + 56 + 4 = 80
	char fname[16];
	int size;
	int direct[14];
	int indirect;
} inode;





/*************************/
//Jnode
typedef struct jnode {

	inode ptrs[200];

        
} jnode;


/*************************/
//Free Bit Map (FBM)
typedef struct FBM {
	
	/* Each Char is 1 byte, so we have 
	we can keep track of here 8*1024 = 8192 blocks.
	We're gonna have to perform some modular arithmetic on each
	char in our block in order to find free spaces.
	I actually don't think that this will be that costly a cpu 
	function to compute for each file creation, probably still much
	less than the io required for creating the actual file */

	char map[1024];
} FBM;



/*************************/
//Write Mask (WM)
typedef struct WM {



} WM;

/*************************/
//SuperBlock
typedef struct superblock {
	
	unsigned char magic[4];
	int b_size;		//Likely 1024
	int fs_size;		//Total File System Size
	int nb_Inodes;
	jnode currentRoot;
	jnode shadowroots[];
	

} superblock;




/********************************************************************************************************************/
//Creates File System
void mkssfs(int fresh) {
	
	int blocksize = BLOCK_SIZE;
	int maxblock = MAX_BLOCK;
	char* diskName = DISK_NAME;

	//New Disk
	if (fresh == 1) {
		int successful = init_fresh_disk(diskName, blocksize, maxblock);
		if (successful != 0)
			perror("Error creating fresh disk");
	}
	else {
		int successful = init_disk(diskName, blocksize, maxblock);
		if (successful != 0)
			perror("Error reloading disk");
	}


	//1. Setup Super Block (jnode)

	//2. Setup Free Bit Map (1 block)

	//3. Setup the Write Mask (1 block)

	//4. Initialize file containing all i-nodes

	//5. Setup root directory (read only)


}







/********************************************************************************************************************/
//Opens/Creates the given file
int ssfs_fopen(char * name) {
	
	/*Note:
	If we are creating a new file, we will need to
	duplicate, modify, and replace our root directory
	file to also contain a pointer to this file */

	//1. (If New) Allocate and Initalize an i-node


	//2. Create entry in Open File Descriptor Table
	int fd = 0;
	
	//3. Write pieces of information to entry
		//3.1 i-node number 
		//3.2 read pointer
		//3.3 write pointer


	return fd;
}








/********************************************************************************************************************/
//Closes the given file
int ssfs_fclose(int fileID) {




	//Successful
	return 0;
}








/********************************************************************************************************************/
//Seek (Read) to the location from the beginning
int ssfs_frseek(int fileID, int loc) {

	//Modify read pointer

	//Success
	return 0;
}









/********************************************************************************************************************/
//Seek (Write) to the location from the beginning
int ssfs_fwseek(int fileID, int loc) {

	//Modify write pointer

	//Success
	return 0;
}








/********************************************************************************************************************/
//Write buf characters into disk
int ssfs_fwrite(int fileID, char* buf, int length) {

	//1. Get Block to write

	//2. Write the block

	return length;
}







/********************************************************************************************************************/
//Read characters from disk into buf
int ssfs_fread(int fileID, char* buf, int length) {



	return length;
}









/********************************************************************************************************************/
//Removes a file from the filesystem
int ssfs_remove(char* file) {

}











/********************************************************************************************************************/
//Create a shadow of the file system
int ssfs_commit() {

	//1. Makes all newly created block copies unwriteable

	




	/* Returns index of the shadow
	root that holds the previous commit */
	int previousRoot = 0;
	return previousRoot;


}











/********************************************************************************************************************/
//Restore the file system to a previous shadow
int ssfs_restore(int cnum) {
	
	//cnum is a previous shadow root index

}

















/********************************************************************************************************************/
//Main
int main(int argc, char* argv[]) {
	
	//Checks if file exists
	char* diskname = DISK_NAME;
	int fresh = 1;
	FILE *fp;
	if (fp = fopen(diskname, "r")) {
		fclose(fp);
		printf("file already exists\n");
		fresh = 0;
	}
	
	//Open disk emulator
	mkssfs(fresh);

	//Closes disk emulator
	int close_disk();
	return 0;
}

