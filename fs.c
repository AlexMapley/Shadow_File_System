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
#define INODE_POINTERS 13;	//starting at index 0
#define INODES_PER_PAGE 16;



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
	int writePointers[200];
	int readPointers[200];

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


/*************************/
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



/*************************/
//Next Free Write Block
int findNextFreeBlock(FBM *freeBitMap, int flag) {

	//First non-allocated block is index 8
	for (int i = 7; i < 1024; i++) {
	
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

			return i;
		}
	}
}



/*************************/
//Next Open File Descriptor
int nextFreeFTB() {

	for (int i = 1; i < 200; i++) {

		/* Returns index i, if
		there is no struct at that index */
		if ( xFileDescriptorTable->openFiles[i].indirect == 0) {		
			return i;
			
		}
	}
}




/*************************/
//Is our file already open?
int findOpenFile(int fileID) {

	int limit = NUMBER_INODES;
	inode* cacheNode = malloc(sizeof(inode));

	/*Scans through file table starting at 2nd index (1),
	caching each stored inode and checking it's id */
	for (int i = 1; i < limit; i++) {	
	

		//Tests if inode exists at this index
		if ( xFileDescriptorTable->openFiles[i].indirect != 0) {		

			//Is it the correct file?
			if (xFileDescriptorTable->openFiles[i].indirect == fileID) {

				printf("MATCH AT INDEX %i\n", i);
				printf("(find) file direct[0] = %i, indirect = %i\n", xFileDescriptorTable->openFiles[i].direct[0], xFileDescriptorTable->openFiles[i].indirect);
		
				
				//Return Inode index in FDT
				free(cacheNode);
				return i;
			}
		}
		
	}	

	//Unsuccesful
	free(cacheNode);
	return -1;
}


/*************************/
//Deep Copy Inode
void inodeDeepCopy(inode *inode1, inode *inode2) {

	//Copy Indirect
	inode1->indirect = inode2->indirect;
	
	//Copy Direct Pointers
	for (int i = 0; i < 14; i++) {
		memcpy(&(inode1->direct[i]), &(inode2->direct[i]), sizeof(int));
	}
}



/*************************************************************************************************************/
/* Actual Method Signature*/


//Creates File System
void mkssfs(int fresh) {

	// Initialize File Descriptor Table
	xFileDescriptorTable = malloc(sizeof(FDT));
	for (int i = 0; i < 200; i++) 
		xFileDescriptorTable->openFiles[i].indirect = 0;


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
		(the root directory) begins at block 8,
		and all other pointers are 0 */
		inode* root = malloc(sizeof(inode));
		memset(root->direct, 0, sizeof(root->direct));
		root -> direct[0] = 7;
		root -> indirect = 1;
		superblock -> currentRoot = *root;
		

		/* 3. Initialize and write the
		Write Mask and Free Bit Map.
		For convinience, I'm going to write these
		at blocks 2 and 3 respectively. Blocks 4-8
		are part of our file lookup directory for filenames.
		We will initialize xFreeBitMap to have the 
		(char) value of 1 for indices 0-3,
		so that no files can be written in those
		regions. This value is set to 1 as 2^0 = 1,
		and it can be assumed that in initialization we 
		are working from shadow root 0. */
		FBM* freeBitMap = malloc(sizeof(FBM));
		for (int i = 0 ; i < 8; i ++) {
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
	will involve scanning through all of our root directory
	pages, and scanning the file names associated with
	each file inode */

	//Scanning Variables
	int blockIndex, pageIndex, rootIndex;
	int chosenBlockIndex, chosenPageIndex;
	int fileFound = 0;
	int foundEmptySpace = 0;
	unsigned char nameBuf[16];
	void *pageBuf[1024];
	unsigned char xname[16];
	strcpy(xname, name);
	int readSize  = sizeof(nameBuf);
	int pageLimit = 64;
	int maxPointers = INODE_POINTERS;


	/*REDUCTION SEARCH
	We only match our file names to our
	saved disk data with
	the first 8 characters in each string
	for comparison. This is the result
	of an incredibly frustrating and elusive bug,
	wherein I can only copy the first 8 characters
	of a name from disk once calling fopen again. */
	char reducedName[] = {xname[0],xname[1],xname[2],xname[3],xname[4],xname[5],xname[6],xname[7], 0};


	//Scan through lookup pages, blocks indexed 4-8
	for (blockIndex = 3; blockIndex < 7; blockIndex++) {

		//Place entire block into buffer
		memset(pageBuf,1,1024);
		read_blocks(blockIndex, 1, pageBuf);
	
	
		//Read each name from block into buffer
		for (pageIndex = 0; pageIndex < pageLimit; pageIndex++) {

			//Read stored name into buffer
			memcpy(&nameBuf[0], &pageBuf[pageIndex * 16], 16);		
			
			//Break Condition, name found
			if(strcmp(nameBuf, reducedName) == 0) {
				printf("\n1. Found filename: '%s' at index %i\n", nameBuf, pageIndex);
				fileFound = 1;
				chosenPageIndex = pageIndex;
				chosenBlockIndex = blockIndex;
				
				break;
			}
			//Found next empty space
			if(strlen(nameBuf) == 0 && foundEmptySpace == 0 && fileFound == 0) {
				foundEmptySpace = 1;
				chosenBlockIndex = blockIndex;
				chosenPageIndex = pageIndex;
			}
		}
		

		//Outer Break, name found
		if (fileFound == 1) 
				break;

	}


/****************************/

	/* 2. (Possibly) Cache Existing Node */
	if (fileFound == 1) {
		
		
		/*Shift block and page index to where
		our node is stored in the disk-space */
		blockIndex = chosenBlockIndex;
		pageIndex = chosenPageIndex;

		//Get location of inode
		int fileNumber = (((blockIndex - 3) *  64) + pageIndex);
		int directPointer = (fileNumber / 16);
		int writeAddress = ((fileNumber % 16) * 64);


		//Cache Root Inode Page
		read_blocks(xRoot->direct[directPointer], 1, pageBuf);

		printf("fileNumber: %i\n", fileNumber);


		//Memcpy into new inode
		memcpy(&newNode, &pageBuf[writeAddress], sizeof(newNode));

		/* 2.1 Checks if file is already open in FDT */
		int openFd = findOpenFile(newNode->indirect);
		if (openFd != -1) {
			//Free newNode and return
			free(newNode);
			return openFd;
		}

		/* 2.2 Else, create entry in Open File Descriptor Table,
		and then return that entry index */
		else {
			int fd = nextFreeFTB();

			//Copy newNode to FDT
			memcpy(&(xFileDescriptorTable -> openFiles[fd]), newNode, sizeof(inode));

			//Set read/write pointers
			xFileDescriptorTable -> readPointers[fd] = 0;		
			xFileDescriptorTable -> writePointers[fd] = 0;

		
			//Return Index
			free(newNode);
			return fd;
		}
	}

/****************************/

	/* 3. Else Create File 					
	Here we need to create a new inode,
	either in our current page or inside a new page.
`	Also, we need to add its name to the 
	lookup pages. */

	else {

		/*Shift block and page index to the empty
		space we found before */
		blockIndex = chosenBlockIndex;
		pageIndex = chosenPageIndex;

		//Place entire block into buffer, again
		read_blocks(blockIndex, 1, pageBuf);

		//Write new file name	
		memcpy(&pageBuf[pageIndex * 16], name, sizeof(name));
		write_blocks(blockIndex, 1, pageBuf);


		/* Find next available spot to store inode
		in root directory pages. 
		Will be ((blockindex - 3) *  64) for our fileNumber,
		(fileNumber / 16) to find our root->direct pointer,
		((fileNumber % 16)-1) to find our next empty spot,
		and thus a write address of ((next empty spot) * 64) */
		int fileNumber = (((blockIndex - 3) *  64) + pageIndex);
		int directPointer = (fileNumber / 16);
		int writeAddress = ((fileNumber % 16) * 64);
		newNode -> indirect = (fileNumber + 1);

		//Initialize inode pointers to 0
		for ( int k = 0; k < 14; k++)
			newNode->direct[k] = 0;
		

		//Do we designate a new root page?
		if (writeAddress == 0) {
			xRoot -> direct[directPointer] = findNextFreeBlock(xFreeBitMap, 1);
		}


		//Cache Root Inode Page
		read_blocks(xRoot->direct[directPointer], 1, pageBuf);

		//Memcpy new inode
		memcpy(&pageBuf[writeAddress], &newNode, sizeof(newNode));

		//Store updated Root Page
		write_blocks(xRoot->direct[directPointer], 1, pageBuf);		

		/* 3.1 Create entry in Open File Descriptor Table,
		and then return that entry index */
		int fd = nextFreeFTB();

		//Copy newNode to FDT
		xFileDescriptorTable -> openFiles[fd] = *newNode;
		
		//Set read/write pointers
		xFileDescriptorTable -> readPointers[fd] = 0;		
		xFileDescriptorTable -> writePointers[fd] = 0;


		//Return Index
		return fd;

	}			
}



/********************************************************************************************************************/
//Closes the given file
int ssfs_fclose(int fileID) {

	/* Helper Method defined above,
	as I use it i na few different functions.
	Checks if the file exists in my FDT,
	and returns its index. If it doesn't exist,
	returns -1 */
	
	printf("\n\nTRYING TO CLOSE INODE #%i\n", fileID);				//TESTER

	int inodeLocation = findOpenFile(fileID);

	
	//No such file
	if (inodeLocation == -1) {
		printf("No such file found\n");
		return -1;
	}
	/* Found our file!
	replace it with an empty iNode */
	else {
	
		inode* shellNode = malloc(sizeof(inode));
		shellNode->indirect = 0;
		xFileDescriptorTable -> openFiles[inodeLocation] = *shellNode;
		xFileDescriptorTable -> readPointers[inodeLocation] = 0;		
		xFileDescriptorTable -> writePointers[inodeLocation] = 0;
		return 0;
	}
	
	return 0;

}








/********************************************************************************************************************/
//Seek (Read) to the location from the beginning
int ssfs_frseek(int fileID, int loc) {

	//Is this an open file?
	if (findOpenFile(fileID) == -1) {
		printf("No such file\n");
		return -1;
	}
	
	//Invalid values
	if (loc < 0 || loc > 14335) {
		printf("Invalid location\n");
		return -1;
	}
	
	/* Everything checks out,
	set location in FDT */
	xFileDescriptorTable -> readPointers[fileID] = loc;

	//Success
	return 0;
}









/********************************************************************************************************************/
//Seek (Write) to the location from the beginning
int ssfs_fwseek(int fileID, int loc) {

	//Is this an open file?
	if (findOpenFile(fileID) == -1) {
		printf("No such file\n");
		return -1;
	}
	
	//Invalid values
	if (loc < 0 || loc > 14335) {
		printf("Invalid location\n");
		return -1;
	}
	
	/* Everything checks out,
	set location in FDT */
	xFileDescriptorTable -> writePointers[fileID] = loc;


	//Success
	return 0;
}








/********************************************************************************************************************/
//Write buf characters into disk
int ssfs_fwrite(int fileID, char* buf, int length) {

	int debugID = fileID;			//Solves some weird memory corruption bug
	int blockSize = BLOCK_SIZE;
	int maxNodes = INODE_POINTERS;

	//Convert input to char array
	unsigned char xBuf[length+1];
	strcpy(xBuf, buf);

	//Is this an open file?
	if (findOpenFile(fileID) == -1) {
		printf("No such file\n");
		return -1;
	}

	
	//Declare buffers
	char pageBuf[blockSize];
	int addr = 0;

	//Find readPointer
	int writePointer = xFileDescriptorTable->writePointers[fileID];
	int startingNode = (writePointer / blockSize);
	int startingByte = (writePointer % blockSize);


	//Cache open Inode from File Descriptor Table
	inode* cacheNode = malloc(sizeof(inode));
	int limit = NUMBER_INODES;

	/*Scans through file table starting at 2nd index (1),
	caching each stored inode and checking it's id */
	for (int i = 1; i < limit; i++) {							

		//Is it the correct file?
		if (xFileDescriptorTable->openFiles[i].indirect == fileID) {
			
			/* Deep Copy Inode at index,
			references prior defined helper method,
			'inodeDeepCopy' */
			inodeDeepCopy(cacheNode, &(xFileDescriptorTable->openFiles[i])); 

			//Break outer loop				
			i+=1024;
		}
	}

	//Does this write require designating a first new block?
	if (cacheNode->direct[startingNode] == 0) {
		cacheNode->direct[startingNode] = findNextFreeBlock(xFreeBitMap, 1);
	}
	
	/* Preform our writing at each block.
	Will we need to designate even more blocks?
	Check the length of our input compared to our
	starting byte, and see if to accomodate an
	input of that length we'll need to designate
	more blocks */
	int tmpNode = startingNode;
	int inputAddr = 0;
	int remainingInputLength = length;
	while(remainingInputLength >= 0 && tmpNode < 14) {
	
		//Read page into buffer
		read_blocks(cacheNode->direct[tmpNode], 1, pageBuf);
		
		//Write to buffer
		memcpy(&pageBuf[(startingByte%1024)], &xBuf[inputAddr], (remainingInputLength % blockSize)); 

		//Adjust Write Pointer
		xFileDescriptorTable->writePointers[debugID] += (remainingInputLength % blockSize);

		//Adjust RemainingInputLength
		remainingInputLength -= (blockSize - startingByte);

		//Adjust inputAddr
		inputAddr =  inputAddr + (blockSize - startingByte);
		
		//Write buffer into page
		write_blocks(cacheNode->direct[tmpNode], 1, pageBuf);
		
		//Allocate next block, if unallocatated
		tmpNode++;
		if (cacheNode->direct[tmpNode] == 0) 
			cacheNode->direct[tmpNode] = findNextFreeBlock(xFreeBitMap, 1);

		//Set startingByte to 0
		startingByte = 0;

		//Clear memory page
		memset(&pageBuf[0], 0, sizeof(pageBuf));
	}

	//Error check
	cacheNode->indirect = debugID;					//TESTER

	//Store Altered Inode in FDT
	for (int i = 1; i < limit; i++) {							

		//Is it the correct file?
		if (xFileDescriptorTable->openFiles[i].indirect == debugID) {
			
			/* Deep Copy Inode at index,
			references prior defined helper method,
			'inodeDeepCopy' */
			inodeDeepCopy(&(xFileDescriptorTable->openFiles[i]), cacheNode); 

			//Break outer loop				
			i+=1024;
		}
	}
	

	/* Store Altered Inode to Disk */
	int pageLimit = 16;
	void *inodeBuf[1024];
	int blockIndex, pageIndex;

	//Get location of inode
	int directPointer = (debugID / 16);
	int writeAddress = ((debugID % 16) * 64);

	//Cache Root Inode Page
	read_blocks(xRoot->direct[directPointer], 1, inodeBuf);

	//Memcpy new inode into buffer
	memcpy(&inodeBuf[writeAddress], &cacheNode , sizeof(cacheNode));





	//Write updated block to disk
	write_blocks(xRoot->direct[directPointer], 1, inodeBuf);

	//Free cacheNode
	free(cacheNode); 

	return length;
}






/********************************************************************************************************************/
//Read characters from disk into buf
int ssfs_fread(int fileID, char* buf, int length) {

	int blockSize = BLOCK_SIZE;
	int maxNodes = INODE_POINTERS;

	//Is this an open file?
	if (findOpenFile(fileID) == -1) {
		printf("No such file\n");
		return -1;
	}

	//Declare buffers
	char pageBuf[blockSize];
	char masterBuf[blockSize * maxNodes];

	//Find readPointer
	int readPointer = xFileDescriptorTable->readPointers[fileID];
	int startingNode = (readPointer / blockSize);
	int startingByte = (readPointer % blockSize);
	printf("readPointer: %i, startingNode: %i, startingByte %i\n", readPointer, startingNode, startingByte);

	//Cache open Inode from File Descriptor Table
	inode* cacheNode = malloc(sizeof(inode));
	int limit = NUMBER_INODES;

	/*Scans through file table starting at 2nd index (1),
	caching each stored inode and checking it's id */
	for (int i = 1; i < limit; i++) {							

		//Is it the correct file?
		if (xFileDescriptorTable->openFiles[i].indirect == fileID) {

			/* Deep Copy Inode at index,
			references prior defined helper method,
			'inodeDeepCopy' */
			inodeDeepCopy(cacheNode, &(xFileDescriptorTable->openFiles[i])); 

			//Break loop				
			i+=1024;
		}
	}

	/* Preform our reading at each block.
	Will stop if we are requesting to
	read input that isn't there */
	int tmpNode = startingNode;
	int inputAddr = 0;
	int remainingInputLength = length;
	int bufIndex = 0;
	while(remainingInputLength >= 0 && tmpNode < 14) {
			
		//Read page into buffer
		read_blocks(cacheNode->direct[tmpNode], 1, pageBuf);
	
		//Write to buffer
		memcpy(&masterBuf[bufIndex], &pageBuf[startingByte], (remainingInputLength % blockSize)); 

		//Adjust Read Pointer
		xFileDescriptorTable->readPointers[fileID] += (remainingInputLength % blockSize);


		//Adjust RemainingInputLength
		remainingInputLength -= (blockSize - startingByte);
		
		//Onto the next theoretical block
		tmpNode++;
	
		//Increment Buf Index
		bufIndex += (blockSize - startingByte);

		//Set startingByte to 0
		startingByte = 0;

		printf("REMAINING INPUT LENGTH: %i\n\n", remainingInputLength);
	}

	printf("masterBuf contents: %s\n\n\n", &masterBuf[0]);

	//malloc(strlen(buf)*sizeof(char));			//UPDATE
	strcpy(buf, masterBuf);
	

	//Free cacheNode
	free(cacheNode);

	return length;
}






/********************************************************************************************************************/
//Removes a file from the filesystem
int ssfs_remove(char* file) {

	printf("REMOVING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");			//TESTER

	/* 1. Find our file in the lookup table.
	Either we will find it and remove it,
	or we will not find it if it doesn't exist
	and return a failure */

	int blockIndex, pageIndex, rootIndex;
	int chosenBlockIndex, chosenPageIndex;
	int fileFound = 0;
	int foundEmptySpace = 0;
	char nameBuf[16];
	void *pageBuf[1024];
	char xname[16];
	strcpy(xname, file);
	int readSize  = sizeof(nameBuf);
	int pageLimit = 64;
	int maxPointers = INODE_POINTERS;

	//Scan through lookup pages, blocks indexed 4-8
	for (blockIndex = 3; blockIndex < 7; blockIndex++) {


		//Place entire block into buffer
		read_blocks(blockIndex, 1, pageBuf);

		
		//Read each name from block into buffer
		for (pageIndex = 0; pageIndex < pageLimit; pageIndex++) {

			
			memcpy(nameBuf, pageBuf + (pageIndex * readSize), sizeof(nameBuf));


			//Break Condition, name found
			if(!(strcmp(nameBuf, xname))) {
				printf("Found filename: '%s'\n", nameBuf);
				fileFound = 1;
				chosenBlockIndex = blockIndex;
				chosenPageIndex = pageIndex;
				break;
			}
			//Found next empty space
			if(strlen(nameBuf) == 0 && foundEmptySpace == 0) {
				foundEmptySpace = 1;
				chosenBlockIndex = blockIndex;
				chosenPageIndex = pageIndex;
			}
		}
		

		//Outer Break, name found
		if (fileFound == 1) {
			break;
		}
	}

	//Makes sure our file exists
	if (fileFound == 0) {
		printf("Can't remove a nonexistant file\n");
		return -1;
	}	
	
	//Remove filename from lookup table
	char cleaner[16];
	memcpy(&pageBuf[pageIndex * 16], cleaner, sizeof(cleaner));
	write_blocks(blockIndex, 1, pageBuf);


/**************************/

	/* 2. Remove this inode from the data pages
	pointed to by our root node */
	inode *cacheNode = malloc(sizeof(inode));
	
	//Set indexes
	blockIndex = chosenBlockIndex;
	pageIndex = chosenPageIndex;

	/* Same Modular Arithmetic for finding
	inodes as in ssfs_fopen*/
	int fileNumber = (((blockIndex - 3) *  64) + pageIndex);
	int directPointer = (fileNumber / 16);
	int writeAddress = ((fileNumber % 16) * 64);

	//Cache Root Inode Page
	read_blocks(xRoot->direct[directPointer], 1, pageBuf);
		
	//Memcpy inode into cache
	memcpy(&cacheNode, &pageBuf[writeAddress], sizeof(cacheNode));


	//Memset inode from page
	memset(&pageBuf[writeAddress], 0, sizeof(cacheNode));

	//Store updated Root Page, with this inode removed
	write_blocks(xRoot->direct[directPointer], 1, pageBuf);	
		
		
/*****************/

// 3. Removes entry from FDT, if open
	if (findOpenFile(cacheNode->indirect) != -1) {
		printf("closing open file\n");
		ssfs_fclose(cacheNode->indirect);
	}

/*****************/

	/* 4. Then, free all data blocks pointed to 
	by the inode. With each block we free, we
	also overwrite our FBM to show that these
	spots are now open */
	for (int k = 0; k < maxPointers; k++) {
			
		/*If block pointer is empty, we 
		free our cacheNode and return */
		if (cacheNode->direct[k] == 0) {
			free(cacheNode);			
			return 0;
		}
		//Erase Block Contents
		char cleaner[1024];
		write_blocks(cacheNode->direct[k], 1, cleaner);

		/* Modify FBM, as always preforming complicated
		modular arithmetic */
		int fbmVal = (int) xFreeBitMap->map[cacheNode->direct[k]];
		fbmVal = fbmVal - powerOfTwo(xSuperBlock->mostRecentShadow);

		//Convert fbmVal back to char and store
		xFreeBitMap->map[cacheNode->direct[k]] = (char) fbmVal;			

	} 

	//Free cacheNode
	free(cacheNode);

	return 0;
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
	return 0;

}

















/********************************************************************************************************************/
//Main
/*

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


/*****************************/
//Open test

	//printf("\nOPEN TESTS:\n");
	//int fd = ssfs_fopen("file1");
/*	fd = ssfs_fopen("file2");
	fd = ssfs_fopen("file3");
	fd = ssfs_fopen("file1");
	fd = ssfs_fopen("file2");
	fd = ssfs_fopen("file3");


/*****************************/
//Close Tests

/*
	ssfs_fclose(1);
	ssfs_fclose(1);
	printf("\n----------------------------------------\nCLOSE TESTS:\n");
	ssfs_fclose(2);
	printf("\nTrying to close file again\n");
	ssfs_fclose(2);


/*****************************/
//Remove tests
	/*printf("\n----------------------------------\nREMOVE TESTS:\n");
	ssfs_remove("file1");
	ssfs_remove("file2");
	ssfs_remove("file3");
	printf("\n----------------------------------\nTRYING TO REMOVE FILES TWICE:\n");
	ssfs_remove("file1");
	ssfs_remove("file2");
	ssfs_remove("file3");
	ssfs_remove("file4");
	

/*****************************/
//Read / Write Tests
/*
printf("\n----------------------------------------\nWRITE TESTS:\n");

char* input = "Hello!";
printf("Input: %s\n", input);
ssfs_fwrite(1, input, sizeof(input));

printf("\n----------------------\nRead TESTS:\n");

char* output = (char *)malloc(strlen(input)+1);
ssfs_fread(1, output, sizeof(input));
printf("Output: %s\n", output);


/*****************************/
/*
printf("\n\n");

	//Closes disk emulator
	int close_disk();

	//Removes file				//TEST PHASE 1
	unlink("MapleDisk");
	return 0;
} */

