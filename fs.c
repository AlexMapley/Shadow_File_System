#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "disk_emu.h"



#define BLOCK_SIZE 1024;
#define MAX_BLOCK 1024;
#define NUMBER_INODES 200;
#define DISK_NAME "MapleDisk"
#define MAX_INODES 14;
#define INODES_PER_PAGE 13;



/********************************************************************************************************************/
/* STRUCTURES IN SYSTEM
Designates the information and design
principles needed for each component in our
shadow file system. At the very least,
this can be used as a reference to 
malloc'd size */



/*************************/
//Inode
typedef struct inode {

	//Size in bytes = 4 + 56 + 4 = 64
	int size;
	int direct[14];
	int indirect;
} inode;


/*************************/
//File Descriptor Table
typedef struct FDT {
	
	inode openFiles[200];

} FDT;






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
	
	/* Each Char is 1 byte, so we have 
	we can keep track of here 8*1024 = 8192 blocks.
	We're gonna have to perform some modular arithmetic on each
	char in our block in order to find free spaces.
	I actually don't think that this will be that costly a cpu 
	function to compute for each file creation, probably still much
	less than the io required for creating the actual file */

	char map[1024];

} WM;


//SuperBlock
typedef struct super_block {
	
	unsigned char magic[4];
	int b_size;		//Likely 1024
	int fs_size;		//Total File System Size
	int nb_Inodes;
	inode currentRoot;
	inode shadowroots[4];
	int mostRecentShadow;
	

} super_block;





/********************************************************************************************************************/
/* Open Global Structures (Cached) */
super_block* xSuperBlock;
FBM* xFreeBitMap;
WM* xWriteMask;
FDT* xFileDescriptorTable;
inode* xRoot;








/********************************************************************************************************************/
/* Miscellaneous Helper Methods */

//Power of Two
int powerOfTwo(int x) {
	int z = 2;
	if (x == 0) 
		return 1;
	
	else if (x == 1)
		return 2;

	else {
		for (int y = 1; y < x; y++)
			z = z * 2;
	}

}

//Next Free Write Block
int findNextFreeBlock(FBM *freeBitMap, int flag) {

	for (int i = 3; i < 1024; i++) {
	
		/* Here we are converting our char value to an integer,
		increasing it by 2^(our shadow current root), and then
		recasting it as a char and storing it back. 
		It looks confusing, but I don't think there's a more condense
		way to do it without storing variables for reference (inneficient!).
		It'd be even worse if I hadn't wrote helper methods... */
		int pageFree = (((int) (xFreeBitMap->map[i])) / powerOfTwo(xSuperBlock->mostRecentShadow)) % 2;
	
		if (pageFree == 0) {
			//If flag is 1, mark this area as written
			if (flag == 1) {
				xFreeBitMap->map[i] = (char) 
				((int) (xFreeBitMap->map[i]) + powerOfTwo(xSuperBlock->mostRecentShadow));
			}

			//Return block index
			return i;
		}
	}
}

//Next Open File Descriptor
int nextFreeFTB() {
	for (int i = 0; i < 200; i++) {
		
		/* Returns index i, if
		there is no struct at that index */
		if ( xFileDescriptorTable->openFiles[i].indirect == 0) 
			return i;
	}
}








/*************************************************************************************************************/
/* Actual Method Signature*/


//Creates File System
void mkssfs(int fresh) {

	//Initialize File Descriptor Table
	xFileDescriptorTable = malloc(sizeof(FDT));

	int blocksize = BLOCK_SIZE;
	int maxblock = MAX_BLOCK;
	char* diskName = DISK_NAME;

	//Case 1. New Disk
	if (fresh == 1) {
		
		/* 1. Bootup Disk */
		int successful = init_fresh_disk(diskName, blocksize, maxblock);
		if (successful != 0)
			perror("Error creating fresh disk");

		/* 2. Declare Super Block,
		and initialize some of its variables */
		super_block* superblock = (super_block*) malloc(sizeof(super_block));
		superblock -> b_size = BLOCK_SIZE;
		superblock -> fs_size = BLOCK_SIZE;
		superblock -> nb_Inodes = NUMBER_INODES;
		superblock -> mostRecentShadow = 0;
		strcpy (superblock -> magic, "Alex");    //magic number?
	
		/*Root inode
		Initialized so that its first data block
		(the root directory) begins at block 4 */
		inode* root = malloc(sizeof(inode));
		root -> direct[0] = 3;
		root -> indirect = 1;
		superblock -> currentRoot = *root;
		

		/* 3. Initialize and write the
		Write Mask and Free Bit Map.
		For convinience, I'm going to write these
		at blocks 2 and 3 respectively. Block 4
		will be the first page of our root directory.
		We will initialize xFreeBitMap to have the 
		(char) value of 1 for indices 0-3,
		so that no files can be written in those
		regions. This value is set to 1 as 2^0 = 1,
		and it can be assumed that in initialization we 
		are working from shadow root 0. */
		FBM* freeBitMap = malloc(sizeof(FBM));
		for (int i = 0 ; i < 4; i ++) {
			freeBitMap -> map[i] = ((char) 1); 
		}

		WM* writeMask = malloc(sizeof(WM));
		
		//Writes each to the disk
		write_blocks(0, 1, superblock);
		write_blocks(1, 1, freeBitMap);
		write_blocks(2, 1, writeMask);


		/* Cache Global Structs:
		We will also store these structures
		in our main meory (running program) */
		xSuperBlock = superblock;
		xFreeBitMap = freeBitMap;
		xWriteMask = writeMask;
		xRoot = root;


		
	}

/* NOT YET REACHED */							//PHASE 2
	//Case 2. Reviving old Disk
	else {
		int successful = init_disk(diskName, blocksize, maxblock);
		if (successful != 0)
			perror("Error reloading disk");

		/* Cache Global Structs:
		We're going to need to read and store
		our superBlock, freeBitMap, and 
		writeMask into our Main Memory (running program) */
		read_blocks(0, 1, &xSuperBlock);
		read_blocks(1, 1, &xFreeBitMap);
		read_blocks(2, 1, &xWriteMask);
		
	
	}

	//Set first open file as root
	xFileDescriptorTable -> openFiles[0] = *xRoot;

}





/********************************************************************************************************************/
//Opens/Creates the given file
int ssfs_fopen(char * name) {
	
	/*Note:
	If we are creating a new file, we will need to
	duplicate, modify, and replace our root directory
	file to also contain a pointer to this file */

	//Create new cached Inode
	inode* newNode = malloc(sizeof(inode));

/****************************/

	/* 1. See if our file exists. This
	will involve scanning through all of our rott directory
	pages, and scanning the file names associated with
	each file inode */

	//Scanning Variables
	int blockIndex, pageIndex, rootIndex;
	char xname[16];
	strcpy(xname, name);
	char nameBuf[16];
	char pageBuf[1024];

	int readSize  = sizeof(inode) + sizeof(nameBuf);

	int pageLimit = INODES_PER_PAGE;
	int maxNodes = MAX_INODES;

	//Actual Scan
	for (rootIndex = 0; rootIndex < pageLimit; rootIndex++) {

		//Get block index from root pointer
		blockIndex = xRoot->direct[rootIndex];
		
		printf("Index: %i\n", xRoot->direct[rootIndex]);				//TESTER

		//Place entire block into buffer
		read_blocks(blockIndex, 1, pageBuf);

		
		//Read each name from block into buffer
		for (pageIndex = 0; pageIndex < pageLimit; pageIndex++) {

			
			strncpy(nameBuf, pageBuf + (pageIndex * readSize), sizeof(nameBuf));


			printf("Page index: %i\n", pageIndex);					//TESTER
	
			//Break Condition 1, name found
			if(!(strcmp(nameBuf, name))) {
				printf("Found filename: '%s'\n", nameBuf);
				break;
			}
			//Break Condition 2, no name stored
			if(strlen(nameBuf) == 0) {
				printf("No matching filename found\n");					//TESTER
				break;
			}
		}
		

		//Outer Break  1, name found
		if (strcmp(nameBuf, name)) 
				break;

		//Outer Break 2, no name stored
		if (strlen(nameBuf) == 0)
				break;
		
	}


/****************************/

	/* 2. (Possibly) Create File 					//BUG: ONLY WORKS AT TOP OF BLOCK
	Here we need to create a new inode,
	either in our current page or inside a new page */

	if (strlen(nameBuf) == 0) {


		/* Do we need to create a new page? 
		references 'findNextFreeBlock' helper method above.
		This will return our next free index, and the flag 1
		will mark this index as now containing data */
		if ( pageIndex == 13) {

			blockIndex = findNextFreeBlock(xFreeBitMap, 1);
			
			//Empty our page buffer, as we're starting a new page
			memset(&pageBuf[0], 0, sizeof(pageBuf));
			pageIndex = 0;
		
		}
			
		
		/* Set Inode pointer to next available block,
		and write this pointer with it's name id into our
		root directory. 
		Also, set the inode's indrect index. */
		newNode -> direct[0] = findNextFreeBlock(xFreeBitMap, 1);
		newNode -> indirect = (((rootIndex+1) * (pageIndex+1)) + 1);
		
		printf("inode number: %i\n",  ((rootIndex+1) * (pageIndex+1) ) + 1);		//TEST

		//Write new iNode to our page buffer
		memcpy(&pageBuf[((pageIndex+1) * 80)], name, sizeof(name));
		memcpy(&pageBuf[((pageIndex+1) * 80) + sizeof(newNode)], newNode, sizeof(newNode));


		
		/* Rewrite this page block, with our updated 
		root directory page */
		write_blocks(blockIndex, 1, pageBuf);					
	
		
		
	}

	/*3. Create entry in Open File Descriptor Table,
	and then return that entry index */
	int fd = nextFreeFTB();
	xFileDescriptorTable -> openFiles[fd] = *newNode;
	
	printf("fd: %i\n", fd);
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
	
	char* diskname = DISK_NAME;
	int fresh = 1;
	FILE *fp;


	if (fp = fopen(diskname, "r")) {
		fclose(fp);
		printf("File system exists\n");
		//fresh = 0;
		
	}

	
	//Open disk emulator
	mkssfs(fresh);


	//Open test
	ssfs_fopen("file1");

	//Closes disk emulator
	int close_disk();

	//Removes file				//TEST PHASE 1
	unlink("MapleDisk");
	return 0;
}

