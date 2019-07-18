# Virtual-Memory-Simulator


## Contributors
* [Mike Curry](https://github.com/mjcurry)
* [Dan Kramer](https://github.com/codankra)


## TO DO
- [x] Structure of pages and page tables
- [x] Page table for all programs
- [x] Inital loading of pages into main memory
- [x] Implement FIFO page replacement algorithm
- [x] Implement LRU page replacement algorithm
- [x] Implement Clock page replacement algorithm
- [x] Implement Pre-Paging
- [x] Experiment and analyze results with different page sizes

## Description

The goal of this project is to simulate a virtual memory management system. The compiled program accepts the following
parameters at the command prompt in the order specified:

1. plist (the name of the process list file)
2. ptrace (the name of the process execution trace file)
3. page size (# of memory locations for each page)
4. FIFO, LRU, or Clock (type of page replacement algorithm)
  - FIFO: first-in first-out
  - LRU: least recently used
  - Clock: clock algorithm
5. + or - (a flag to toggle a pre-paging feature)
  - +: turn pre-paging on
  - -: leave default demand paging

Two files, `plist` and `ptrace` are provided this repo, and can be used for testing. An example command looks like this:

`VMsimulator plist ptrace 2 FIFO +`

Which is the compiled program `VMsimulator` simulating the processes in `ptrace` with a page size of 2, and a FIFO page replacement algorithm with pre-paging.


When run the program will:
1. Simulate a paging system
2. Implement the three different page replacement algorithms
3. Handle a variable page size specified by the user
4. Implement both demand and pre-paging
5. Record the number of page swaps that occured during a run


### Simulation of a Paging System

In this simulation, consider a *memory location* to be an atomic unit, that is, the smallest possible unit we care to consider. Thus, in a system with a page size of 2, there are two memory locations on each page.

The program's main memory holds **512** memory locations

Two files are supplied for the purpose of example:
- plist
  - Contains a list of programs that will be loaded into main memory.
  - Each line has the format `(pid, total # of memory locations)` which specifies the total number of memory locations needed by each program
- ptrace
  - Contains a deterministic series of memory accesses which emulate a real system's memory usage.
  - Each line has the format `(pid, referenced memory location)`, which specifies the memory location requested by the program.

#### plist and Initial Loading

Each page in each page table will have a name or number (which is *unique* with respect to all pages accross all page tables) so it can quickly determine whether it is present in main memory or not. The size of each page table is decided by the number of pages in the program. It is calculated by dividing the total number of memory locations of the program (found in `plist`) by the page size (from input parameters).

Thus, page tables are represented by the following data structure:

| Page number | Valid bit         | Last time accessed |
|-------------|-------------------|--------------------|
| N1          | 0 (not in memory) | T1                 |
| N2          | 1 (in memory)     | T2                 |
| ...         | ...               | ...                |

Once we have the page tables, it will perform a default loading of memory before starting to read the pages as indicated in `ptrace`. That is, we will load an initial resident set of each program's page table into main memory. *The main memory is divided equally among all programs in plist*. If the program doesn't have enough pages for its default load, it will leave its unused load blank. After initializing memory allocation, it will update the page tables (i.e. set valid bit of corresponding pages in table to 1) according to the page assignment. *If it doesn't divide evenly, it will keep the leftover memory*.

#### ptrace

Finally, the program will begin reading in `ptrace`. Each line of this file represents a memory location request within a program. It will need to translate this location into the unique page number that it has stored in the page tables made later, and determine if the requested page is in memory or not. If it is, it will simply continue with the next command in ptrace. If not, it will record that a page swap was made, and initiate a page replacement algorithm. **For each program, the pages to be replaced are picked from those pages allocated to itself (which is called local page allocation policy).**

![](paging_model.png?raw=true)


### Implementation of Page Replacement Algorithms

- First in, First Out (FIFO): The oldest page in memory will be the first to leave when a page swap is performed.
- Least Recently Used (LRU): The page with the oldest access time will be replaced in memory when a page swap is performed.
- Clock based policy: The simple version of this algorithm which only uses one bit

#### Implementation Details

For each of these algorithms, `time()` and `clock()` are not sufficiently sensitive to timestamp memory accesses. Instead a global counter which keeps track of memory accesses (as a relative measure of age) is used.

#### Page of Variable Sizes

This affects not only how many pages each program will take up, but also the "size" of main memory. For example, if the page size is 4 then main memory will have 128 available page spots. This simulation is able to use page sizes that are powers of 2 up to a max of 32, and other sizes are out of scope.

#### Demand and pre-paging

Demand paging replaces 1 page with the requested page during a page fault

Pre-paging brings 2 pages into memory for every swap: the requested page and the next contiguous page which is not loaded in memory. Thus, if the next contiguous page in a table is already in memory, it will keep looking for the next page until it reaches one that isn't in memory or until all pages have been checked.

#### Record of Page Swaps

Anytime a memory location is read, it will be translated to a page number. If it is not found in main memory, a page swap must be initiated. This is a metric of each algorithm's performance.
