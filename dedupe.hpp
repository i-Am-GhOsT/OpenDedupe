#ifndef _DEDUP_DRIVER_H_
#define _DEDUP_DRIVER_H_

#include "common.hpp"

typedef struct dedup_s {
  int chunk_size;
  std::string file_name;
} dedup_t;

static map<unsigned long, std::string> chunk_hash;
static pthread_mutex_t hashMutex;
static int thread_count;
  
void* dedupFile(void *);
void dump_dedup_map (std::string path);
__inline__ unsigned long calculateHash(char *str);


#endif // _DEDUP_DRIVER_H
  
