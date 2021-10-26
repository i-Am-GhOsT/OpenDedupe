#ifndef _COMMON_H_
#define _COMMON_H_



#include "stdio.h"
#include "string.h"
#include <cstring>
#include <map>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <typeinfo>
#include <chrono>
#include <unistd.h>


using namespace std;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;


#ifndef MAX_THREAD
#  define MAX_THREAD 5
#endif

static std::string checkPath(std::string path) {
  
  if(path[path.length() - 1] == '/') 
    path = path.substr(0, path.length() - 1);
  
  return path;
}

int dedupeDriver(std::string dir_to_dedupe, long chunk_size);
int rehydrateDriver(std::string dir_to_dedupe, long chunk_size);

#endif
