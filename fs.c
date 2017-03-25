#include <stdin.h>







/********************************************************************************************************************/
//Creates File System
void mkssfs(int fresh) {

//To do:

//1. Setup Super Block

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

