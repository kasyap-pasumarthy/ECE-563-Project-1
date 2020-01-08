#include <string>

#include <iostream>

#include <math.h>

#include <fstream>

#include <sstream>

#include <bitset>

#include <math.h>

#include <cstring>

#include <sstream>

#include <stdio.h>

#include <stdlib.h>

#include <iomanip>


using namespace std;

int r_ops = 0, w_ops = 0, read_hits = 0, read_misses = 0, write_hits = 0, write_misses = 0, writebacks = 0, L2_calls = 0, L2_reads = 0, L2_writes = 0, L2read_misses = 0, L2read_hits = 0, L2write_hits = 0, L2write_misses = 0, L2writebacks = 0, L2_block_misses = 0, L2_sec_read = 0, L2_sec_read_hits = 0, L2_sec_read_misses = 0, L2_sec_write = 0, L2_sec_write_hits = 0, L2_sec_write_misses = 0, L2_sector_misses = 0, L2_cache_misses = 0, L2_sec_writebacks = 0;

class Cache {
   public:
      unsigned int tag;
   bool valid;
   bool dirty;
   int LRU;
   unsigned int address;
}; //end class cache

Cache L1[4096][4096]; //creating a blank 2D array
Cache L2[3072][3072]; //creating a blank 2D array

class Cache_sectored_address {

   public:
      unsigned int tag;
};

Cache_sectored_address L2_address[3072][3072];
class Cache_sectored_data {

   public:
      bool valid;
   bool dirty;
   unsigned int selector;
};

Cache_sectored_data L2_data[3072][3072];

void cacheSimL2Sectored(unsigned int address, char op_type, int addTags, int dataBlocks, int blockSize, int num_sets) {

   int offset_bits, tag_bits, index_bits, addTag_bits, dataBlock_bits;

   offset_bits = log2(blockSize);
   dataBlock_bits = log2(dataBlocks);
   index_bits = log2(num_sets);
   addTag_bits = log2(addTags);
   tag_bits = 32 - (offset_bits + dataBlock_bits + index_bits + addTag_bits);

   unsigned int index, dataBlockIndex, addTagIndex, mask, mask2, mask3, mask4, a, b, c, d, tag;
   a = address >> offset_bits;
   b = a >> dataBlock_bits;
   c = b >> index_bits;
   d = c >> addTag_bits;

   mask = 4294967295;
   //mask1 = mask >> (offset_bits + dataBlock_bits + index_bits + addTag_bits);
   mask2 = mask >> (offset_bits + index_bits + addTag_bits + tag_bits);
   mask3 = mask >> (offset_bits + dataBlock_bits + addTag_bits + tag_bits);
   mask4 = mask >> (offset_bits + dataBlock_bits + index_bits + tag_bits);

   dataBlockIndex = a & mask2;
   index = b & mask3;
   addTagIndex = c & mask4; // selector *********
   tag = d;

   //   cout<<"tag "<<hex<< tag<< endl;
   //cout<< "set number" << dec<<index<<endl;
   //cout<< "selector " << dec<<addTagIndex<<endl;
   //cout<< "data block index " << dec<<dataBlockIndex<<endl;

   if (op_type == 'r') { //read end
      L2_sec_read++;
      int hit = 0;

      if (L2_address[index][addTagIndex].tag == tag && L2_data[index][dataBlockIndex].valid == 1 && L2_data[index][dataBlockIndex].selector == addTagIndex) {
         hit = 1;
      }

      if (hit == 1) {
         L2_sec_read_hits++;
      } else if (hit == 0) { //read miss
         L2_sec_read_misses++;
         int sec_miss = 0, ctr = 0;

         for (int i = 0; i < dataBlocks; i++) {

            if (L2_data[index][i].valid == 0) {
               ctr++;
            }
         }

         if (ctr == dataBlocks) {
            sec_miss = 1;
         } else {
            sec_miss = 0;
         }

         if (sec_miss == 1) { //sector miss

            L2_sector_misses++;
            L2_address[index][addTagIndex].tag = tag;
            L2_data[index][dataBlockIndex].valid = 1;
            L2_data[index][dataBlockIndex].dirty = 0;
            L2_data[index][dataBlockIndex].selector = addTagIndex;

         } //sector miss end
         else if (sec_miss == 0) { //cache miss

            L2_cache_misses++;

            if (L2_address[index][addTagIndex].tag == tag) { //tag matches 

               if (L2_data[index][dataBlockIndex].valid == 0) { //invalid

                  L2_data[index][dataBlockIndex].valid = 1;
                  L2_data[index][dataBlockIndex].selector = addTagIndex;

               } //invalid end
               else if (L2_data[index][dataBlockIndex].valid == 1 && L2_data[index][dataBlockIndex].selector != addTagIndex) { //valid but wrong selector

                  L2_data[index][dataBlockIndex].valid = 1;
                  L2_data[index][dataBlockIndex].selector = addTagIndex;
                  if (L2_data[index][dataBlockIndex].dirty == 1) {
                     L2_sec_writebacks++;
                     L2_data[index][dataBlockIndex].dirty = 0;
                  }

               } //valid end

            } //tag matches end
            else if (L2_address[index][addTagIndex].tag != tag) { //tag mismatch

               L2_address[index][addTagIndex].tag = tag;

               for (int i = 0; i < dataBlocks; i++) { //invalidating and setting

                  if (L2_data[index][i].selector == addTagIndex) { //begin selector check

                     if (L2_data[index][i].valid == 1 && L2_data[index][i].dirty == 1) {
                        L2_sec_writebacks++;
                        L2_data[index][i].dirty = 0;
                     }

                     L2_data[index][i].valid = 0;
                     L2_data[index][i].selector = 0;

                  } //end selector check

               } //end invalidating and setting

               if (L2_data[index][dataBlockIndex].valid == 1 && L2_data[index][dataBlockIndex].dirty == 1) {
                  L2_sec_writebacks++;
                  L2_data[index][dataBlockIndex].dirty = 0;
               }
               //else if(L2_data[index][dataBlockIndex].valid != 1){L2_data[index][dataBlockIndex].dirty = 0;}
               //else if(L2_data[index][dataBlockIndex].valid == 1 && L2_data[index][dataBlockIndex].dirty != 1){L2_data[index][dataBlockIndex].dirty =0;}

               L2_data[index][dataBlockIndex].valid = 1;
               L2_data[index][dataBlockIndex].selector = addTagIndex;

            } //tag mismatch end

         } //cache miss end

      } //read miss end

   } //read end
   else if (op_type == 'w') { //write begin
      L2_sec_write++;
      int hit = 0;

      if (L2_address[index][addTagIndex].tag == tag && L2_data[index][dataBlockIndex].valid == 1 && L2_data[index][dataBlockIndex].selector == addTagIndex) {
         hit = 1;
      }

      if (hit == 1) {
         L2_sec_write_hits++;
         L2_data[index][dataBlockIndex].dirty = 1;
      } else if (hit == 0) { //read miss
         L2_sec_write_misses++;
         int sec_miss = 0, ctr = 0;

         for (int i = 0; i < dataBlocks; i++) {

            if (L2_data[index][i].valid == 0) {
               ctr++;
            }
         }

         if (ctr == dataBlocks) {
            sec_miss = 1;
         } else {
            sec_miss = 0;
         }

         if (sec_miss == 1) { //sector miss

            L2_sector_misses++;
            L2_address[index][addTagIndex].tag = tag;
            L2_data[index][dataBlockIndex].valid = 1;
            L2_data[index][dataBlockIndex].dirty = 1;
            L2_data[index][dataBlockIndex].selector = addTagIndex;

         } //sector miss end
         else if (sec_miss == 0) { //cache miss

            L2_cache_misses++;

            if (L2_address[index][addTagIndex].tag == tag) { //tag matches 

               if (L2_data[index][dataBlockIndex].valid == 0) { //invalid

                  L2_data[index][dataBlockIndex].valid = 1;
                  L2_data[index][dataBlockIndex].dirty = 1;
                  L2_data[index][dataBlockIndex].selector = addTagIndex;

               } //invalid end
               else if (L2_data[index][dataBlockIndex].valid == 1 && L2_data[index][dataBlockIndex].selector != addTagIndex) { //valid but wrong selector

                  L2_data[index][dataBlockIndex].valid = 1;
                  L2_data[index][dataBlockIndex].selector = addTagIndex;
                  if (L2_data[index][dataBlockIndex].dirty == 1) {
                     L2_sec_writebacks++;
                  }
                  L2_data[index][dataBlockIndex].dirty = 1;

               } //valid end

            } //tag matches end
            else if (L2_address[index][addTagIndex].tag != tag) { //tag mismatch

               L2_address[index][addTagIndex].tag = tag;

               for (int i = 0; i < dataBlocks; i++) { //invalidating and setting

                  if (L2_data[index][i].selector == addTagIndex) { //begin selector check

                     if (L2_data[index][i].valid == 1 && L2_data[index][i].dirty == 1) {
                        L2_sec_writebacks++;
                     }

                     L2_data[index][i].valid = 0;
                     L2_data[index][i].selector = 0;
                     L2_data[index][i].dirty = 0;

                  } //end selector check

               } //end invalidating and setting

               if (L2_data[index][dataBlockIndex].valid == 1 && L2_data[index][dataBlockIndex].dirty == 1) {
                  L2_sec_writebacks++;
                  L2_data[index][dataBlockIndex].dirty = 1;
               } else if (L2_data[index][dataBlockIndex].valid != 1) {
                  L2_data[index][dataBlockIndex].dirty = 1;
               } else if (L2_data[index][dataBlockIndex].valid == 1 && L2_data[index][dataBlockIndex].dirty != 1) {
                  L2_data[index][dataBlockIndex].dirty = 1;
               }

               L2_data[index][dataBlockIndex].valid = 1;
               L2_data[index][dataBlockIndex].selector = addTagIndex;

            } //tag mismatch end

         } //cache miss end

      } //read miss end
   } //write end
} //sectoredL2end

void LRUupdateL2(int set, int block_num, int assoc) { //LRU counter updating

   for (int j = 0; j < assoc; j++) {
      if (L2[set][j].LRU < L2[set][block_num].LRU) {

         L2[set][j].LRU += 1;
      }
   }

   //resetting LRU of hit block

   L2[set][block_num].LRU = 0;

} //end LRU update

int findLRUL2(int index, int L2cacheAssoc) { //finding LRU block to evict

   int LRUblock = 0;
   for (int i = 0; i < L2cacheAssoc; i++) {
      int ctr = 0;
      for (int j = 0; j < L2cacheAssoc; j++) {

         if (L2[index][i].LRU >= L2[index][j].LRU) {
            ctr++;
         }
      }

      if (ctr == L2cacheAssoc) {
         LRUblock = i;
      }
   }

   return LRUblock;
} //extracting LRU
void cacheSimL2(unsigned int address, char opType, int tag_bits, int index_bits, int offset_bits, int L2cacheSize, int blockSize, int L2cacheAssoc) { //begin L2, add op_type later
   unsigned int L2index, aL2, tagL2, mask;
   //cout<<"L2 called"<<endl;

   if (opType == 'r') {

      L2_reads++;

      aL2 = address >> offset_bits;
      mask = 4294967295;
      mask = mask >> (tag_bits + offset_bits);
      if (L2cacheAssoc < (L2cacheSize / blockSize)) {
         L2index = (address >> offset_bits) & mask;
      } else if (L2cacheAssoc >= (L2cacheSize / blockSize)) {
         L2index = 0;
      }
      //Tag
      tagL2 = aL2 >> index_bits;

      int hit = 0, foundAt = 0;
      for (int i = 0; i < L2cacheAssoc; i++) {
         if (L2[L2index][i].tag == tagL2) {
            hit = 1;
            foundAt = i;
         }
      }

      if (hit == 1) {
         L2read_hits++;
         LRUupdateL2(L2index, foundAt, L2cacheAssoc);
      } else if (hit == 0) {
         L2read_misses++;
         int invalidBlockAt = 0;
         int invalidFound = 0;
         for (int i = 0; i < L2cacheAssoc; i++) {
            if (L2[L2index][i].valid == 0) {
               invalidBlockAt = i;
               invalidFound = 1;
            }
         }

         if (invalidFound == 1) {
            L2[L2index][invalidBlockAt].tag = tagL2;
            LRUupdateL2(L2index, invalidBlockAt, L2cacheAssoc);
            L2[L2index][invalidBlockAt].valid = 1;
            L2[L2index][invalidBlockAt].dirty = 0;
         } else if (invalidFound == 0) {

            int LRUblock = 0;
            LRUblock = findLRUL2(L2index, L2cacheAssoc);

            L2[L2index][LRUblock].tag = tagL2;
            LRUupdateL2(L2index, LRUblock, L2cacheAssoc);
            L2[L2index][LRUblock].valid = 1;
            if (L2[L2index][LRUblock].dirty == 1) {
               L2writebacks++;
               L2[L2index][LRUblock].dirty = 0;
            } else if (L2[L2index][LRUblock].dirty == 0) {
               L2[L2index][LRUblock].dirty = 0;
            }

         }
      }

   } //end read
   else if (opType == 'w') {

      L2_writes++;
      aL2 = address >> offset_bits;
      mask = 4294967295;
      mask = mask >> (tag_bits + offset_bits);
      if (L2cacheAssoc < (L2cacheSize / blockSize)) {
         L2index = (address >> offset_bits) & mask;
      } else if (L2cacheAssoc >= (L2cacheSize / blockSize)) {
         L2index = 0;
      }
      //Tag
      tagL2 = aL2 >> index_bits;

      int hit = 0, foundAt = 0;
      for (int i = 0; i < L2cacheAssoc; i++) {
         if (L2[L2index][i].tag == tagL2) {
            hit = 1;
            foundAt = i;
         }
      }

      if (hit == 1) {
         L2write_hits++;
         LRUupdateL2(L2index, foundAt, L2cacheAssoc);
         L2[L2index][foundAt].dirty = 1;
      } else if (hit == 0) {

         L2write_misses++;
         int invalidBlockAt = 0;
         int invalidFound = 0;
         for (int i = 0; i < L2cacheAssoc; i++) {
            if (L2[L2index][i].valid == 0) {
               invalidBlockAt = i;
               invalidFound = 1;
            }
         }

         if (invalidFound == 1) {

            L2[L2index][invalidBlockAt].tag = tagL2;
            LRUupdateL2(L2index, invalidBlockAt, L2cacheAssoc);
            L2[L2index][invalidBlockAt].valid = 1;
            L2[L2index][invalidBlockAt].dirty = 1;
         } else if (invalidFound == 0) {

            int LRUblock = findLRUL2(L2index, L2cacheAssoc);

            L2[L2index][LRUblock].tag = tagL2;
            LRUupdateL2(L2index, LRUblock, L2cacheAssoc);
            L2[L2index][LRUblock].valid = 1;
            if (L2[L2index][LRUblock].dirty == 1) {
               L2writebacks++;
               L2[L2index][LRUblock].dirty = 1;
            } else if (L2[L2index][LRUblock].dirty == 0) {
               L2[L2index][LRUblock].dirty = 1;
            }

         }
      }
   } //end write

   //cout<<"L2 called "<<dec<<L2_calls<<" times."<<endl;
   //L2_calls++;
   //cout<<"L2 tag"<< dec<<tag<<endl;
   //cout<<"L2 index"<< dec<<index<<endl;
   //cout<<"L2 assoc"<< dec<<L2assoc<<endl;

} //end L2
void LRUupdate(int set, int block_num, int assoc) { //LRU counter updating

   for (int j = 0; j < assoc; j++) {
      if (L1[set][j].LRU < L1[set][block_num].LRU) {

         L1[set][j].LRU += 1;
      }
   }

   //resetting LRU of hit block

   L1[set][block_num].LRU = 0;

} //end LRU update

int findLRU(int index, int L1cacheAssoc) { //finding LRU block to evict

   int LRUblock = 0;
   for (int i = 0; i < L1cacheAssoc; i++) {
      int ctr = 0;
      for (int j = 0; j < L1cacheAssoc; j++) {

         if (L1[index][i].LRU >= L1[index][j].LRU) {
            ctr++;
         }
      }

      if (ctr == L1cacheAssoc) {
         LRUblock = i;
      }
   }

   return LRUblock;
} //extracting LRU

void cacheSimL1(const char * c, unsigned int address, int index_bits, int offset_bits, int tag_bits, int L1cacheAssoc, int L1cacheSize, int blockSize, int L2index_bits, int L2offset_bits, int L2tag_bits, int L2cacheAssoc, int L2cacheSize, int L2cacheBlocks, int L2cacheTags, int L2num_sets) {

   int sectored = 0;

   if (L2cacheBlocks > 1 && L2cacheTags > 1) {
      sectored = 1;
   }

   unsigned int index, mask, a, tag;
   a = address >> offset_bits;
   mask = 4294967295;
   mask = mask >> (tag_bits + offset_bits);
   if (L1cacheAssoc < (L1cacheSize / blockSize)) {
      index = (address >> offset_bits) & mask;
   } else if (L1cacheAssoc >= (L1cacheSize / blockSize)) {
      index = 0;
   }
   //Tag
   tag = a >> index_bits;

   //unsigned int  aL2;
   //aL2 = address >> L2offset_bits;
   mask = 4294967295;
   mask = mask >> (L2tag_bits + L2offset_bits);
   if (L1cacheAssoc < (L2cacheSize / blockSize)) {
      // L2index = (address >> offset_bits) & mask;
   } else if (L2cacheAssoc >= (L2cacheSize / blockSize)) {
      //L2index = 0;
   }
   //Tag
   //tagL2 = aL2 >> L2index_bits;

   //cout<<"L2 stats"<<endl;
   //cout<<"L2 index bits"<<dec<<L2index_bits<<endl;
   //cout<<"L2 offset bits"<<dec<<L2offset_bits<<endl;
   //cout<<"L2 tag bits"<<dec<<L2tag_bits<<endl;
   //cout<<"L2 index"<<dec<<L2index<<endl;
   //cout<<"L2 tag"<<dec<<tagL2<<endl;

   if ( * c == 'r' || * c == 'R') {
      r_ops++;
      int hit = 0, foundAt = 0;
      for (int i = 0; i < L1cacheAssoc; i++) {
         if (L1[index][i].tag == tag) {
            hit = 1;
            foundAt = i;
         }
      }

      if (hit == 1) {
         read_hits++;
         LRUupdate(index, foundAt, L1cacheAssoc);
      } else if (hit == 0) {
         read_misses++;

         int invalidBlockAt = 0;
         int invalidFound = 0;
         for (int i = 0; i < L1cacheAssoc; i++) {
            if (L1[index][i].valid == 0) {
               invalidBlockAt = i;
               invalidFound = 1;
            }
         }

         if (invalidFound == 1) {

            L1[index][invalidBlockAt].tag = tag;
            L1[index][invalidBlockAt].address = address;
            LRUupdate(index, invalidBlockAt, L1cacheAssoc);
            L1[index][invalidBlockAt].valid = 1;
            L1[index][invalidBlockAt].dirty = 0;
            if (L2cacheSize != 0) {
               if (sectored == 0) {
                  cacheSimL2(address, 'r', L2tag_bits, L2index_bits, L2offset_bits, L2cacheSize, blockSize, L2cacheAssoc); //read req from l2

               } else if (sectored == 1) {
                  cacheSimL2Sectored(address, 'r', L2cacheTags, L2cacheBlocks, blockSize, L2num_sets);
               }
            }
         } else if (invalidFound == 0) {

            int LRUblock = 0;
            LRUblock = findLRU(index, L1cacheAssoc);
            unsigned int L2address = L1[index][LRUblock].address;
            L1[index][LRUblock].tag = tag;
            L1[index][LRUblock].address = address;
            LRUupdate(index, LRUblock, L1cacheAssoc);
            L1[index][LRUblock].valid = 1;

            if (L1[index][LRUblock].dirty == 1) {
               writebacks++;
               L1[index][LRUblock].dirty = 0;
               if (L2cacheSize != 0) {
                  if (sectored == 0) {
                     cacheSimL2(L2address, 'w', L2tag_bits, L2index_bits, L2offset_bits, L2cacheSize, blockSize, L2cacheAssoc); //write back req
                     cacheSimL2(address, 'r', L2tag_bits, L2index_bits, L2offset_bits, L2cacheSize, blockSize, L2cacheAssoc); //read req from l2
                  } else if (sectored == 1) {
                     cacheSimL2Sectored(L2address, 'w', L2cacheTags, L2cacheBlocks, blockSize, L2num_sets);
                     cacheSimL2Sectored(address, 'r', L2cacheTags, L2cacheBlocks, blockSize, L2num_sets);
                  }
               }
            } else if (L1[index][LRUblock].dirty == 0) {
               L1[index][LRUblock].dirty = 0;
               if (L2cacheSize != 0) {
                  if (sectored == 0) {
                     cacheSimL2(address, 'r', L2tag_bits, L2index_bits, L2offset_bits, L2cacheSize, blockSize, L2cacheAssoc); //read req from l2
                  } else if (sectored == 1) {
                     cacheSimL2Sectored(address, 'r', L2cacheTags, L2cacheBlocks, blockSize, L2num_sets);
                  }
               }
            }

         }
      }

   }

   //end read ops
   else if ( * c == 'w' || * c == 'W') {
      w_ops++;
      int hit = 0, foundAt = 0;
      for (int i = 0; i < L1cacheAssoc; i++) {
         if (L1[index][i].tag == tag) {
            hit = 1;
            foundAt = i;
         }
      }

      if (hit == 1) {
         write_hits++;
         LRUupdate(index, foundAt, L1cacheAssoc);
         L1[index][foundAt].dirty = 1;
      } else if (hit == 0) {

         write_misses++;
         int invalidBlockAt = 0;
         int invalidFound = 0;
         for (int i = 0; i < L1cacheAssoc; i++) {
            if (L1[index][i].valid == 0) {
               invalidBlockAt = i;
               invalidFound = 1;
            }
         }

         if (invalidFound == 1) {

            L1[index][invalidBlockAt].tag = tag;
            //  unsigned int L2address = L1[index][invalidBlockAt].address;
            L1[index][invalidBlockAt].address = address;
            LRUupdate(index, invalidBlockAt, L1cacheAssoc);
            L1[index][invalidBlockAt].valid = 1;
            L1[index][invalidBlockAt].dirty = 1;
            if (L2cacheSize != 0) {
               if (sectored == 0) {
                  cacheSimL2(address, 'r', L2tag_bits, L2index_bits, L2offset_bits, L2cacheSize, blockSize, L2cacheAssoc); //read req from l2
               } else if (sectored == 1) {
                  cacheSimL2Sectored(address, 'r', L2cacheTags, L2cacheBlocks, blockSize, L2num_sets);
               }
            }
         } else if (invalidFound == 0) {

            int LRUblock = findLRU(index, L1cacheAssoc);
            unsigned int L2address = L1[index][LRUblock].address;
            L1[index][LRUblock].tag = tag;
            L1[index][LRUblock].address = address;
            LRUupdate(index, LRUblock, L1cacheAssoc);
            L1[index][LRUblock].valid = 1;

            //read req from l2
            if (L1[index][LRUblock].dirty == 1) {
               writebacks++;
               L1[index][LRUblock].dirty = 1;
               if (L2cacheSize != 0) {
                  if (sectored == 0) {
                     cacheSimL2(L2address, 'w', L2tag_bits, L2index_bits, L2offset_bits, L2cacheSize, blockSize, L2cacheAssoc); //write back req
                     cacheSimL2(address, 'r', L2tag_bits, L2index_bits, L2offset_bits, L2cacheSize, blockSize, L2cacheAssoc);
                  } else if (sectored == 1) {
                     cacheSimL2Sectored(L2address, 'w', L2cacheTags, L2cacheBlocks, blockSize, L2num_sets);
                     cacheSimL2Sectored(address, 'r', L2cacheTags, L2cacheBlocks, blockSize, L2num_sets);
                  }
               }

            } else if (L1[index][LRUblock].dirty == 0) {
               L1[index][LRUblock].dirty = 1;
               if (L2cacheSize != 0) {
                  if (sectored == 0) {
                     cacheSimL2(address, 'r', L2tag_bits, L2index_bits, L2offset_bits, L2cacheSize, blockSize, L2cacheAssoc);
                  } else if (sectored == 1) {
                     cacheSimL2Sectored(address, 'r', L2cacheTags, L2cacheBlocks, blockSize, L2num_sets);
                  }
               }

            }

         }
      }

   }

}

int main(int argc, char * argv[]) {
   //inputs

   int L1cacheAssoc;
   int L1cacheSize;
   int blockSize;
   int L2cacheSize;
   int L2cacheAssoc;
   int L2cacheBlocks;
   int L2cacheTags;
   string trace_file;

   blockSize = atoi(argv[1]);
   L1cacheSize = atoi(argv[2]);
   L1cacheAssoc = atoi(argv[3]);
   L2cacheSize = atoi(argv[4]);
   L2cacheAssoc = atoi(argv[5]);
   L2cacheBlocks = atoi(argv[6]);
   L2cacheTags = atoi(argv[7]);
   trace_file = argv[8];

   //calculate number of sets and blocks

   //calculating index tag and offset bits
   int index_bits;
   int L2index_bits;
   int offset_bits;
   int L2offset_bits;
   int tag_bits;
   int L2tag_bits;
   int num_sets, L2num_sets;

   //L1 calcs
   if (L1cacheAssoc < (L1cacheSize / blockSize)) {
      //num_blocks_per_set = L1cacheAssoc;
      num_sets = (L1cacheSize / (blockSize * L1cacheAssoc));
   } else if (L1cacheAssoc >= (L1cacheSize / blockSize)) {
      //num_blocks_per_set = L1cacheAssoc;
      num_sets = 1;
   }

   if (L1cacheAssoc < (L1cacheSize / blockSize)) {
      //OFFSET num of bits
      offset_bits = log2(blockSize);
      //INDEX num of bits
      index_bits = log2(L1cacheSize / (blockSize * L1cacheAssoc));
      //TAG num of bits
      tag_bits = 32 - offset_bits - index_bits;
   } else if (L1cacheAssoc >= (L1cacheSize / blockSize)) {
      offset_bits = log2(blockSize);
      index_bits = 0;
      tag_bits = 32 - offset_bits - index_bits;
   }

   //L2calcs

   if (L2cacheAssoc < (L2cacheSize / blockSize)) {
      //L2num_blocks_per_set = L2cacheAssoc;
      L2num_sets = (L2cacheSize / (blockSize * L2cacheAssoc * L2cacheBlocks));
   } else if (L2cacheAssoc >= (L2cacheSize / blockSize)) {
      // L2num_blocks_per_set = L2cacheAssoc;
      L2num_sets = 1;
   }

   if (L2cacheAssoc < (L2cacheSize / blockSize)) {
      //OFFSET num of bits
      L2offset_bits = log2(blockSize);
      //INDEX num of bits
      L2index_bits = log2(L2cacheSize / (blockSize * L2cacheAssoc));
      //TAG num of bits
      L2tag_bits = 32 - L2offset_bits - L2index_bits;
   } else if (L2cacheAssoc >= (L2cacheSize / blockSize)) {
      L2offset_bits = log2(blockSize);
      L2index_bits = 0;
      L2tag_bits = 32 - L2offset_bits - L2index_bits;
   }
   ifstream infile;
   string line;

   for (int i = 0; i < num_sets; i++) { //initialise L1 to 0
      for (int j = 0; j < L1cacheAssoc; j++) {
         L1[i][j].tag = 0;
         L1[i][j].LRU = j;
         L1[i][j].dirty = 0;
         L1[i][j].valid = 0;

      }
   }

   for (int i = 0; i < L2num_sets; i++) { //initialise L2 to 0
      for (int j = 0; j < L2cacheAssoc; j++) {
         L2[i][j].tag = 0;
         L2[i][j].LRU = j;
         L2[i][j].dirty = 0;
         L2[i][j].valid = 0;

      }
   }

   for (int i = 0; i < L2num_sets; i++) { //initialise L2 to 0
      for (int j = 0; j < L2cacheBlocks; j++) {

         L2_data[i][j].dirty = 0;
         L2_data[i][j].valid = 0;
         L2_data[i][j].selector = 0;

      }
   }

   for (int i = 0; i < L2num_sets; i++) { //initialise L2 to 0
      for (int j = 0; j < L2cacheTags; j++) {
         L2_address[i][j].tag = 0;
      }
   }

   string sub1, sub2;
   unsigned int address;
   infile.open(trace_file.c_str());
   while (getline(infile, line)) {

      istringstream s(line);

      s >> sub1;
      s >> sub2;

      istringstream buffer(sub2);

      buffer >> hex >> address;

      //Send r/w address to L1 cache
      cacheSimL1(sub1.c_str(), address, index_bits, offset_bits, tag_bits, L1cacheAssoc, L1cacheSize, blockSize, L2index_bits, L2offset_bits, L2tag_bits, L2cacheAssoc, L2cacheSize, L2cacheBlocks, L2cacheTags, L2num_sets);

   };
   infile.close();

   //PRINT THE STUFF

   cout << "  ===== Simulator configuration =====" << endl;
   cout << "  BLOCKSIZE:                        " << blockSize << endl;
   cout << "  L1_SIZE:                          " << L1cacheSize << endl;
   cout << "  L1_ASSOC:                         " << L1cacheAssoc << endl;
   cout << "  L2_SIZE:                          " << L2cacheSize << endl;
   cout << "  L2_ASSOC:                         " << L2cacheAssoc << endl;
   cout << "  L2_DATA_BLOCKS:                   " << L2cacheBlocks << endl;
   cout << "  L2_ADDRESS_TAGS:                  " << L2cacheTags << endl;
   cout << "  trace_file:                       " << trace_file << endl;

   cout << endl;

   unsigned int temptag;
   int tempLRU;
   bool tempvalid;
   bool tempdirty;

   for (int ind = 0; ind < num_sets; ind++) { //L1 sort
      for (int i = 0; i < L1cacheAssoc; ++i)

      {

         for (int j = i + 1; j < L1cacheAssoc; ++j)

         {

            if (L1[ind][i].LRU > L1[ind][j].LRU)

            {

               temptag = L1[ind][j].tag; //repeat for others

               L1[ind][j].tag = L1[ind][i].tag;

               L1[ind][i].tag = temptag;

               tempLRU = L1[ind][j].LRU; //repeat for others

               L1[ind][j].LRU = L1[ind][i].LRU;

               L1[ind][i].LRU = tempLRU;

               tempvalid = L1[ind][j].valid; //repeat for others

               L1[ind][j].valid = L1[ind][i].valid;

               L1[ind][i].valid = tempvalid;

               tempdirty = L1[ind][j].dirty; //repeat for others

               L1[ind][j].dirty = L1[ind][i].dirty;

               L1[ind][i].dirty = tempdirty;

            }

         }

      }

   }

   for (int ind = 0; ind < L2num_sets; ind++) { //L2 sort
      for (int i = 0; i < L2cacheAssoc; ++i)

      {

         for (int j = i + 1; j < L2cacheAssoc; ++j)

         {

            if (L2[ind][i].LRU > L2[ind][j].LRU)

            {

               temptag = L2[ind][j].tag; //repeat for others

               L2[ind][j].tag = L2[ind][i].tag;

               L2[ind][i].tag = temptag;

               tempLRU = L2[ind][j].LRU; //repeat for others

               L2[ind][j].LRU = L2[ind][i].LRU;

               L2[ind][i].LRU = tempLRU;

               tempvalid = L2[ind][j].valid; //repeat for others

               L2[ind][j].valid = L2[ind][i].valid;

               L2[ind][i].valid = tempvalid;

               tempdirty = L2[ind][j].dirty; //repeat for others

               L2[ind][j].dirty = L2[ind][i].dirty;

               L2[ind][i].dirty = tempdirty;

            }

         }

      }

   }

   cout << "===== L1 contents =====" << endl;
   for (int i = 0; i < num_sets; i++) {
      cout << "set	" << dec << i << ":	";
      for (int j = 0; j < L1cacheAssoc; j++) {

         cout << hex << L1[i][j].tag << " ";
         // cout << dec<<L1[i][j].LRU << " ";
         if (L1[i][j].dirty == 0) {
            cout << "N";
         } else if (L1[i][j].dirty == 1) {
            cout << "D";
         };

         cout << " || ";
      }
      cout << endl;
   }
   cout << endl;

   float missrate = (float)(read_misses + write_misses) / (r_ops + w_ops);
   int mem_traffic = writebacks;
   float L2missrate = (float)(L2read_misses) / (L2_reads);
   int L2memaccess;
   if (L2cacheSize == 0) {

      float missrate = (float)(read_misses + write_misses) / (r_ops + w_ops);
      int mem_traffic = read_misses + write_misses + writebacks;
      cout << "===== Simulation Results =====" << endl;
      cout << "a. number of L1 reads:			" << dec << r_ops << endl;
      cout << "b. number of L1 read misses:		" << dec << read_misses << endl;
      cout << "c. number of L1 writes:			" << dec << w_ops << endl;
      cout.precision(4);
      cout.unsetf(ios::floatfield);
      cout.setf(ios::fixed, ios::floatfield);
      cout << "d. number of L1 write misses:		" << dec << write_misses << endl;

      cout << "e. L1 miss rate:			" << dec << missrate << endl;
      cout.unsetf(ios::floatfield);
      cout << "f. number of writebacks from L1 memory:	" << dec << writebacks << endl;
      cout << "g. total memory traffic:		" << dec << mem_traffic << endl;
   }
   if (L2cacheTags == 1 && L2cacheBlocks == 1 && L2cacheSize != 0) {
      cout << "===== L2 contents =====" << endl;
      for (int i = 0; i < L2num_sets; i++) {
         cout << "set	" << dec << i << ":	";
         for (int j = 0; j < L2cacheAssoc; j++) {

            cout << hex << L2[i][j].tag << " ";
            // cout << dec<<L1[i][j].LRU << " ";
            if (L2[i][j].dirty == 0) {
               cout << "N";
            } else if (L2[i][j].dirty == 1) {
               cout << "D";
            };

            cout << " || ";
         }
         cout << endl;
      }
      cout << endl;

      if (L2cacheSize != 0) {
         L2memaccess = L2read_misses + L2write_misses + L2writebacks;
      } else {
         L2memaccess = read_misses + write_misses;
      }
      cout << "===== Simulation Results =====" << endl;
      cout << "a. number of L1 reads:			" << dec << r_ops << endl;
      cout << "b. number of L1 read misses:		" << dec << read_misses << endl;
      cout << "c. number of L1 writes:			" << dec << w_ops << endl;
      cout.precision(4);
      cout.unsetf(ios::floatfield);
      cout.setf(ios::fixed, ios::floatfield);
      cout << "d. number of L1 write misses:		" << dec << write_misses << endl;

      cout << "e. L1 miss rate:			" << dec << missrate << endl;

      cout << "f. number of writebacks from L1 memory:       " << dec << mem_traffic << endl;
      cout << "g. number of L2 reads:		" << dec << L2_reads << endl;
      cout << "h. number of L2 read misses:		" << dec << L2read_misses << endl;
      cout << "i. number of L2 writes:		" << dec << L2_writes << endl;
      cout << "j. number of L2 write misses:		" << dec << L2write_misses << endl;
      cout << "k. L2 miss rate:                 " << dec << L2missrate << endl;
      cout << "l. number of writebacks from L2 memory:       " << dec << L2writebacks << endl;
      cout << "m. total memory traffic:              " << dec << L2memaccess << endl;
      cout.unsetf(ios::floatfield);
   } else {
      if (L2cacheSize != 0) {
         cout << "===== L2 Address Array contents =====" << endl;
         for (int i = 0; i < L2num_sets; i++) {
            cout << "set	" << dec << i << ":	";
            for (int j = 0; j < L2cacheTags; j++) {

               cout << hex << L2_address[i][j].tag << " ";

            }
            cout << " || ";
            cout << endl;
         }
         cout << endl;

         cout << "===== L2 Data Arraycontents =====" << endl;

         for (int i = 0; i < L2num_sets; i++) {
            cout << "set	" << dec << i << ":	";
            for (int j = 0; j < L2cacheBlocks; j++) {

               cout << dec << L2_data[i][j].selector << ", ";

               if (L2_data[i][j].valid == 0) {
                  cout << "I" << ", ";
               } else if (L2_data[i][j].valid == 1) {
                  cout << "V" << ", ";
               }

               if (L2_data[i][j].dirty == 0) {
                  cout << "N" << "		 ";
               } else if (L2_data[i][j].dirty == 1) {
                  cout << "D" << "		 ";
               }

            }
            cout << " || ";
            cout << endl;
         }
         cout << endl;
         float L2_sec_missrate = (float)(L2_sec_read_misses) / (L2_sec_read);
         int L2_sec_access;

         L2_sec_access = L2_sec_read_misses + L2_sec_write_misses + L2_sec_writebacks;

         cout << "===== Simulation Results =====" << endl;
         cout << "a. number of L1 reads:			" << dec << r_ops << endl;
         cout << "b. number of L1 read misses:		" << dec << read_misses << endl;
         cout << "c. number of L1 writes:			" << dec << w_ops << endl;
         cout.precision(4);
         cout.unsetf(ios::floatfield);
         cout.setf(ios::fixed, ios::floatfield);
         cout << "d. number of L1 write misses:		" << dec << write_misses << endl;

         cout << "e. L1 miss rate:			" << dec << missrate << endl;
         cout << "f. number of writebacks from L1 memory:       " << dec << mem_traffic << endl;
         cout << "g. number of L2 reads:       " << dec << L2_sec_read << endl;
         cout << "h. number of L2 read misses:       " << dec << L2_sec_read_misses << endl;
         cout << "i. number of L2 writes:       " << dec << L2_sec_write << endl;
         cout << "j. number of L2 write misses:       " << dec << L2_sec_write_misses << endl;
         cout << "k. number of L2 sector misses:       " << dec << L2_sector_misses << endl;
         cout << "l. number of L2 cache block misses:       " << dec << L2_cache_misses << endl;

         cout << "m. L2 miss rate:       " << dec << L2_sec_missrate << endl;
         cout.unsetf(ios::floatfield);
         cout << "n. number of writebacks from L2 memory:       " << dec << L2_sec_writebacks << endl;
         cout << "o. total memory traffic:       " << dec << L2_sec_access << endl;
      }
   }
   return 0;
};
