#include "WB.h"

unsigned int lg2pow2(uint64_t pow2){ // helper function for integer lg2; using (double) version from math is not safe
  unsigned int retval=0;
  while(pow2 != 1 && retval < 64/* -- should actually check the local VA bits, but, as seen below, if !64, will exit anyway...*/){
    pow2 = pow2 >> 1;
    ++retval;
  }
  return retval;
}

void  setSizesOffsetsAndMaskFields(cache* acache, unsigned int size, unsigned int assoc, unsigned int blocksize){
  unsigned int localVAbits=8*sizeof(uint64_t*);
  if (localVAbits!=64){
    fprintf(stderr,"Running non-portable code on unsupported platform, terminating. Please use designated machines.\n");
    exit(-1);
  }

  acache->numways=assoc; // sets associativity
  acache->blocksize=blocksize; // sets blocksize
  acache->numsets=size/(blocksize*assoc); // computes total number of sets
  acache->BO = lg2pow2(blocksize); // computes block offset
  acache->TO = acache->BO + lg2pow2(acache->numsets); // computes tag offset
  acache->VAImask= acache->numsets - 1; // computes AND mask for index
  acache->VATmask=(uint64_t)(((uint64_t)1)<<(localVAbits - acache->TO)) - (uint64_t)1; // computes AND mask for tag
}


unsigned long long getindex(cache* acache, unsigned long long address){
  return (address>>acache->BO) & (uint64_t)acache->VAImask; //Returns index bits, masking upper bits
}

unsigned long long gettag(cache* acache, unsigned long long address){
  return (address>>acache->TO) & (uint64_t)acache->VATmask; //Returns tag bits, masking upper bits
}

void writeback(cache* acache, unsigned int index, unsigned int oldestway){
  if (acache->nextcache) { //check if next cache actually exists, for completeness sake
    cache *nextlev = acache->nextcache; // definition of the next level cache(closest cache)
    uint64_t baseaddress = (acache->sets[index].blocks[oldestway].tag << acache->TO) | (index << acache->BO); //base address calculation (Where are we starting??)

    for (unsigned int i = 0; i < (acache->blocksize / 8); i++) {  // We iterate through the line
      //call performaccess with values corresponding to the iteration
      performaccess(nextlev, baseaddress + i * 8, 8, 1, acache->sets[index].blocks[oldestway].datawords[i], i);
    }
  }
}


void fill(cache * acache, unsigned int index, unsigned int oldestway, unsigned long long address){
  if (acache->nextcache) {  //check if next level of cache exists, for completeness sake
    cache *nextlev = acache->nextcache; // definition of the next level cache(closest cache)
    uint64_t baseaddress = (gettag(acache, address) << acache->TO) | (index << acache->BO);//base address calculation (Where are we starting??)

    for (unsigned int i = 0; i < (acache->blocksize / 8); i++) {
      //call performaccess with values corresponding to the iteration AND assign it to the appropriate line
      acache->sets[index].blocks[oldestway].datawords[i] = performaccess(nextlev, baseaddress + i * 8, 8, 0, 0, i);
    }

  }
}




