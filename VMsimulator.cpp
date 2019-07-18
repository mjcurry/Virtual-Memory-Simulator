#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include<map>
using namespace std;

int NUM_ADDRESSES = 512;
unsigned long TimeOfAccess = 1; //set then increment
int CurPageNum = 0; //set then increment

map<int, int> psizes;

struct page {
	int pageNum;
	int validBit;
	unsigned long lastTimeAccessed;
};

int lookupMemoryLoc(int pid, int loc, int pagesize){
	// function to look up the page number of specicified memory location within a process
	// pid: the process number
	// loc: the memory location within above process
	// pagesize: the page size of the current virtual system
	
	if (loc < 0 || loc > psizes[pid] ){ // first check to see if the memory location is valid for this process
		throw std::invalid_argument("Error: Recieved out-of-bounds memory location for specified process");
	}
	int result = (int)ceil((float)((float)loc/(float)pagesize)) - 1;
	return result;
	
}


int main(int argc, char** argv){
	//VMsimulator plist ptrace 2 FIFO +
	if (argc != 6) {
		printf("Invalid number of command line arguments\n");
		return(-1);
	}
	int pageSize = atoi(argv[3]);
	int algCase = -1;
	string algo = argv[4];
	if (algo.compare("FIFO") == 0) {
		algCase = 0;
	} else if (algo.compare("LRU") == 0) {
		algCase = 1;
	} else if (algo.compare("Clock") == 0) {
		algCase = 2;
	}else{
		printf("Invalid parameter value for page replacement algorithm\n");
		exit(-1);
	}

	string flag = argv[5]; //flag to toggle pre-paging

	bool prePaging;
	bool notYetPrepaged = true;
	if (flag == "+")
		prePaging = true;
	else if (flag == "-")
		prePaging = false;
	else{
		printf("Invalid parameter value for pre-paging [+/-]\n");
		exit(-1);
	}
	
	vector<page**> table; //table for all programs in plist

	//Start file IO------------------
	ifstream plist (argv[1], ifstream::in);
	// vars for tokenizing the plist input file
	int plistLines = 0; //each real plist line is a program, so this is our program counter
	char c;	
	char lastChar = '\n';
	bool readPID = false;
	string currPID = "";
	int pid = 0;
	string currSize = "";
	int size = 0;
	int numPages;
	while(plist.good()){ // while we are still reading file contents
		c = plist.get();	// get the next char in the file buffer		
		//--!!The above while loop and character reading initializes the page table!!--
		if (c == '\n' && lastChar != '\n'){	// if we've reached the end of a line...
			size = atoi(currSize.c_str());	// convert the string we've concatonated to an int
			pid = atoi(currPID.c_str());
			// save the number of memory locations for each process in a map
			psizes[pid] = size;
			numPages = (int)(ceil(((float)size/(float)pageSize)));
			page** anythingreally = new page*[numPages];//initialize "rows" of vector (page* array)
			for(int i = 0; i<numPages; i++){
				anythingreally[i] = new page;
				anythingreally[i]->pageNum = CurPageNum;
				CurPageNum++;
				anythingreally[i]->validBit = 0;
				anythingreally[i]->lastTimeAccessed = 0;
			}
			table.push_back(anythingreally);
			//delete[] anythingreally;
			plistLines++;	// increment programs counter

			//printf("PID Number: %d, Memory Size: %d\n", pid, size);
			currSize = "";	// reset vars for next line
			currPID = "";
			readPID = false;
		}
		else if (!readPID && c != ' '){	// if we are at the start of a newline, read the id until whitespace
			currPID = currPID + c;
		}
		else if (c == ' '){
			readPID = true; // we are done reading in the id, now time to read the size
		}
		else {  // read the number of memory locations until end of line
			currSize = currSize + c;
		}
		lastChar = c; // record last read character to prevent counting blank lines
	}
	plist.close();
	//for our main program:
	numPages = NUM_ADDRESSES/pageSize;
	int pagesPerProgram = numPages/plistLines;


	page* mainMemory[numPages]; //table for our main program
	for (int i = 0; i < plistLines; ++i){ //get from all programs
		for (int j = 0; j < pagesPerProgram; ++j){ //get first pages in program
			mainMemory[j+pagesPerProgram*i] = new page;
			mainMemory[j+pagesPerProgram*i] = table[i][j];
			table[i][j]->validBit = 1; //set page as in memory
			table[i][j]->lastTimeAccessed = TimeOfAccess; //update time accessed
			TimeOfAccess++;
		}
	}
	numPages = pagesPerProgram * plistLines; // main memory should have this amount of pages

	//PTRACE
	ifstream ptrace (argv[2], ifstream::in);
	// vars for tokenizing the plist input file
	lastChar = '\n';
	readPID = false;
	currPID = "";
	pid = 0;
	currSize = "";
	size = 0;
	int clockIndex = 0; //pointer for clock
	int ptraceLines = 0;
	//printf("Finished Plist, now ptracing\n");
	int faultCounter = 0;
	int indexToSwap = 0;
	int tempsize;
	while(ptrace.good()){ // do the whole page replacement process
		c = ptrace.get();			
		if (c == '\n' && lastChar != '\n'){	// if we've reached the end of a line...
			/*
			Steps: 
			1. see if page is in mm
			2. call replace algo to find replace index
			3. swap pages
			*/
			size = atoi(currSize.c_str());	// convert the string we've concatonated to an int
			pid = atoi(currPID.c_str());

			//location of page, no error checking: (int)(ceil(((float)size/(float)pageSize)) - 1)
			//location w/ error checking:
			if (table[pid][lookupMemoryLoc(pid,size, pageSize)]->validBit == 0){ // PAGE FAULT!^!
				faultCounter++;

				prepagecase : //This is how we replace twice in case of pre paging flag being +

				if(algCase == 0){
					//option 1: using time of access as time of entry
					//option 2: using linked list instead of array
					unsigned long minTime = TimeOfAccess;
					int minIndex = 0;
					for (int i = 0; i < numPages; ++i){
						if (minTime>mainMemory[i]->lastTimeAccessed) {
							minIndex = i;
							minTime = mainMemory[i]->lastTimeAccessed;
						}
					}
					indexToSwap = minIndex;
					//printf("CalledFIFO\n");
				}
				else if (algCase == 1){
					unsigned long minTime = TimeOfAccess;
					int minIndex = 0;
					for (int i = 0; i < numPages; ++i){
						if (minTime>mainMemory[i]->lastTimeAccessed) {
							minIndex = i;
							minTime = mainMemory[i]->lastTimeAccessed;
						}
					}
					indexToSwap = minIndex;
					//printf("CalledLRU\n");
				}
				else if(algCase == 2){
					while(mainMemory[clockIndex]->validBit == 1){
						// we are using a value of two to specify that it's still in memory but no longer valid,
						// so the next time we get their it will remove it from memory
						mainMemory[clockIndex]->validBit = 2;
						clockIndex = (clockIndex + 1) % numPages;
					}
					indexToSwap = clockIndex;
					clockIndex = (clockIndex + 1) % numPages;
					//printf("CalledClock\n");
				}
				//GENERAL code for all algorithms
				//set conditional page to memory:
				mainMemory[indexToSwap]->validBit = 0;
				int checkPageNum = mainMemory[indexToSwap]->pageNum;
				if (notYetPrepaged){ //swap with normal location
					mainMemory[indexToSwap] = table[pid][lookupMemoryLoc(pid,size, pageSize)];
					mainMemory[indexToSwap]->validBit = 1; //set page as in memory
					mainMemory[indexToSwap]->lastTimeAccessed = TimeOfAccess; //update time accessed
				} else { //swap with prepaging location
					mainMemory[indexToSwap] = table[pid][lookupMemoryLoc(pid,tempsize, pageSize)];
					mainMemory[indexToSwap]->validBit = 1; //set page as in memory
					mainMemory[indexToSwap]->lastTimeAccessed = TimeOfAccess; //update time accessed
				}
				TimeOfAccess++;

				if (prePaging && notYetPrepaged){ //In this case, do prepaging
					notYetPrepaged = false;
					tempsize = (size+pageSize) % psizes[pid];
					if(tempsize > size && table[pid][lookupMemoryLoc(pid,tempsize, pageSize)]->pageNum == checkPageNum){
						tempsize = (tempsize+pageSize) % psizes[pid];
					} //if initial tempsize is same page as just replaced, move to next page
					while (tempsize > size && table[pid][lookupMemoryLoc(pid,tempsize, pageSize)]->validBit == 1){ 
					//This loop checks for next out-of-memory page and make sure we don't loop back to beginning, also making sure we don't add back in the page we just replaced
						tempsize = (tempsize+pageSize) % psizes[pid];
						if(tempsize > size && table[pid][lookupMemoryLoc(pid,tempsize, pageSize)]->pageNum == checkPageNum){
							tempsize = (tempsize+pageSize) % psizes[pid];
						}//make sure to not end on page you replaced
					}
					if(tempsize > size){
						goto prepagecase;
					}
				}
			} else if (algCase == 1){ //endif - NO PAGE FAULT
				//page in memory
				//update comparative time of access (this is the distinction between lru and fifo)
				table[pid][lookupMemoryLoc(pid,size, pageSize)]->lastTimeAccessed = TimeOfAccess;
				TimeOfAccess++;
				//printf("No replacement necessary.\n");
			} else if (table[pid][lookupMemoryLoc(pid,size, pageSize)]->validBit == 2) {
				table[pid][lookupMemoryLoc(pid,size, pageSize)]->validBit = 1;
			} else {
				//printf("No replacement necessary.\n");
			}

			notYetPrepaged = true;
			//Administrative parsing stuff
			ptraceLines++;	// increment counter
			//printf("PID: %d, Requesting Memory Location: %d\n", pid, size);
			currSize = "";	// reset vars for next line
			currPID = "";
			readPID = false;
		}
		else if (!readPID && c != ' '){	// if we are at the start of a newline, read the id until whitespace
			currPID = currPID + c;
		}
		else if (c == ' '){
			readPID = true; // we are done reading in the id, now time to read the size
		}
		else {  // read the number of memory locations until end of line
			currSize = currSize + c;
		}
		lastChar = c; // record last read character to prevent counting blank lines
	}
	ptrace.close();
	printf("Page Faults: %d\n", faultCounter);
	return 0;
}
