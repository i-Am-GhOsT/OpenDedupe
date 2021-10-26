#include "dedupe.hpp"

dedup_t dedup[MAX_THREAD];

// __inline__ int calculateHash(std::string chunk, long chunk_size) {
  
//   int hashval = 0, i = 0;
  
//   for(i = 0; i < chunk_size; i++) 
//     hashval += chunk[i];
  
//   return hashval;
// }

// djb2
__inline__ unsigned long calculateHash(char *str) {
  
  unsigned long hash = 5381;
  int c;
  
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  
  //printf("Hash calculaated : %lu", hash);

  return hash;
}

void dump_dedup_map (std::string path) {
  
  std::string dump_file = path + "/" + "dedupe_data.csv";
  fstream file_csv;
  
  file_csv.open(dump_file.c_str(), ios::out | ios::app| ios::in);
  if (!file_csv) {
    printf("Error in creating file!!! \n");
    return;
  }
  
  printf ("Dumping map to file : %s \n", dump_file.c_str());
  
  for (auto const &pair : chunk_hash) {
    file_csv << pair.first << "|" << pair.second << endl;
  }
  
  return;
}



void* dedupFile(void *args) {
  
  dedup_t * dedup = (dedup_t *) args;
  
  char *buffer = (char *) malloc (dedup->chunk_size * sizeof(char) + 1);
  
  std::fstream fp;
  fp.open((dedup->file_name).c_str(), ios::out | ios::in);				
  if (!fp.is_open()) {
    printf("File:%s is already open!!! \n", (dedup->file_name).c_str());
    pthread_exit(NULL);
  }
  
  //std::string out_file = dedup->file_name + ".dedup";
  std::string out_file = dedup->file_name + ".dedup";
  std::ofstream ofp;
  ofp.open(out_file, ios::app);
  if (!ofp.is_open()) {
    printf("File:%s is already open!!! \n", out_file.c_str());
    pthread_exit(NULL);
  }
  
  while (fp.read((char *) buffer, dedup->chunk_size)) {
    
    //buffer[chunk_size] = '\0';
    
    unsigned long hashValue = 0; 
    hashValue = calculateHash (buffer);
    
    while (pthread_mutex_trylock(&hashMutex) != 0) {
      // wait - treated as spin lock in this example
      pthread_yield();
      usleep (5000); // sleep for 5milli
    } 
    
    for(int i = 0; i < (dedup->chunk_size+1); i++) {
      
      if(int(buffer[i]) == 13)
	buffer[i]='!';
      
      if(int(buffer[i]) == 10)
	buffer[i]='$';
    }
    
    if (chunk_hash.empty()) {
      printf("Hash is empty..inserting : <key:%lu, value:%s> \n", hashValue, buffer);
      chunk_hash.insert(pair<unsigned long, string> (hashValue, buffer));
    }
    // check if the chunk is present in the chunk hash map.
    // if present do nothing else insert in the chunk hash map
    else if (chunk_hash.find(hashValue) == chunk_hash.end()) {
      // chunk is not present 
      printf("Hash not present..inserting : <key:%lu, value:%s> \n", hashValue, buffer);
      chunk_hash.insert(pair<unsigned long, string> (hashValue, buffer));
    }
    else {
      printf("Hash present..not inserting : <key:%lu, value:%s> \n", hashValue, buffer);
    }
    
    pthread_mutex_unlock(&hashMutex);
    
    ofp << "@" << hashValue << "#\n";
  }
  
  if(fp) {
    printf("File is successfully read!!! \n");
  } else {
    printf("Last %d char left for reading buffer:%s\n", (int) fp.gcount(), buffer);
    ofp << "%" ;
    ofp.write(buffer, fp.gcount());
  }
  
  fp.close();
  ofp.close();
  remove((dedup->file_name).c_str());
  rename(out_file.c_str(), (dedup->file_name).c_str());
  free(buffer);
  
  __sync_fetch_and_sub (&thread_count, 1);
  
  pthread_exit(NULL);
  
  return 0;
}

int dedupeDriver (std::string dir_to_dedupe, long chunk_size) {
  
  struct dirent *dir;
  
  std::string ignore_file = ".dedup";
  static int recursion_count = 0;
  
  DIR *dp = opendir(dir_to_dedupe.c_str());
  if(dp) {
    
    while ((dir = readdir(dp)) != NULL) {
      
      string strpath = "";
      string dir_name = dir->d_name;
      
      if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
	continue;
      
      if (dir->d_type != DT_DIR && 
	  (strcmp(dir->d_name, "dedupe_data.csv") == 0 ||
	   dir_name.compare(dir_name.size() - ignore_file.size(), ignore_file.size(), ignore_file) == 0))
	continue;
      
      if (dir->d_type == DT_DIR) {
	dir_to_dedupe = checkPath(dir_to_dedupe);
	strpath = dir_to_dedupe + "/" + dir->d_name;
	
	recursion_count ++;
	
	if (dedupeDriver (strpath, chunk_size) != 0)
	  return -1;
	
	continue;
      }
      else {
	dir_to_dedupe = checkPath(dir_to_dedupe);
	strpath = dir_to_dedupe + "/" + dir->d_name;
      }
      
      // struct stat result;
      
      // if(stat(strpath.c_str(), &result)==0) {
      // 	auto mod_time = result.st_mtime*1000;
      // 	auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      // 	auto thirty_days=millisec_since_epoch-2592000000;
      
      // 	if(mod_time>=thirty_days)
      // 	  continue;
      // }
      
      __sync_synchronize();
      
      while (thread_count >= MAX_THREAD) {
	printf ("Thread count in greater : %d \n", thread_count);
	sleep(1);
      }
      
      dedup[thread_count].chunk_size = chunk_size;
      dedup[thread_count].file_name = strpath;
      
      if (pthread_mutex_init(&hashMutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return -1;
      }
      
      pthread_t ptid;
      printf("Creating thread for reading file :%s \n", strpath.c_str());
      
      // Creating a new thread
      if (pthread_create(&ptid, NULL, &dedupFile, (void *) &dedup[thread_count]) != 0) {
	printf("Error in creating thread..Stopping dedupe.. \n");
	closedir(dp);
	return -1;
      }
      
      __sync_fetch_and_add (&thread_count, 1);
      pthread_yield();
    }
    
    if (recursion_count == 0) {
      while(thread_count > 0 ) {
	printf ("Thread count wait for all thread : %d \n", thread_count);
	sleep(5);
	__sync_synchronize();
      }
      pthread_mutex_destroy(&hashMutex);
      dump_dedup_map(dir_to_dedupe);
    }
    closedir(dp);
  }
  recursion_count --;
  
  return 0;
}
