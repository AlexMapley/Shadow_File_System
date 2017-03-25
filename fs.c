#include <stdio.h>



#define blocksize 1024;




/********************************************************************************************************************/
/* STRUCTURES IN SYSTEM
Designates the information and design
principles needed for each component in our
shadow file system. At the very least,
this can be used as a reference to 
malloc'd size */

/*************************/
//SuperBlock
struct superblock {
	
	int fsSize;
	int nbInodes;
	//jnode currentRoot;
	//jnode *shadowroots[];
	

} superblock;



//File
struct file {

	char data[4096];
	
} file;




/*************************/
//Inode
 struct inode {

	int size;
	int rdPtr;
	int wrPtr;

} inode;





/*************************/
//Jnode
 struct jnode {

	//inode *ptrs [];

        
} jnode;


/*************************/
//Free Bit Map (FBM)
struct FBM {
	
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
struct WM {



} WM;


/********************************************************************************************************************/
//Creates File System
void mkssfs(int fresh) {

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



	return 0;
}

