Cache Simulator

In this project I made a two level cache simulator with LRU and WBWA policies. The included traces were used to run the simulator and check against the validation runs. The report was only made to satisfy the grading requirements and as such does not go into detail regarding the simulator itself.

Additional functionality was implemented in the form of a direct mapped, decoupled sectored cache.

To run the simulator, type 'make' in console to run the Makefile and compile the code. Once that's done, give command line inputs as follows:

./sim_cache (BLOCKSIZE) (L1_size) (L1_ASSOC) (L2_SIZE) (L2_ASSOC) (L2_DATA_BLOCKS) (L2_ADDR_TAGS) (tracefile)

• BLOCKSIZE: Number of bytes in a block.

• L1_SIZE: Total bytes of data storage for L1 Cache.

• L1_ASSOC: Associativity of L1 cache (ASSOC=1 is a direct-mapped cache)

• L2_SIZE: Total bytes of data storage for L2 Cache

• L2_ASSOC: Associativity of L2 cache

• L2_DATA_BLOCKS: Number of data blocks in a sector 

• L2_ADDR_TAGS: Number of address tags pointing to a sector 

(<tracefile> is the filename of the input trace.) 

The simulator outputs everything to console.