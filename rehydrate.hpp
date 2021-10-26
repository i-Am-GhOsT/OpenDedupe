#ifndef _REHYDRATE_DRIVER_H_
#define _REHYDRATE_DRIVER_H_

#include "common.hpp"

typedef struct dedup_s {
  int chunk_size;
  std::string file_name;
} dedup_t;

static map<unsigned long, std::string> chunk_hash;
static int thread_count;
  
static void* rehydrateFile(void *);
void load_dedup_map (std::string path);

#endif
