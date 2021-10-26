#include "rehydrate.hpp"

dedup_t rehydrate[MAX_THREAD];

void load_dedup_map (std::string path) {
  
  std::string dump_file = path + "/" + "dedupe_data.csv";
  fstream fp;
  fp.open(dump_file.c_str(), ios::out | ios::in);
  if (!fp) {
    printf("Dedupe dump file not found!!! \n");
    exit(1);
  }
  
  unsigned long hash = 0;
  std::string chunk, myline;
  char pipe = '|';
  
  while (fp) {
    size_t pos = 0;
    std::getline (fp, myline);
    
    if (myline == "")
      continue;
    
    pos = myline.find(pipe);
    //hash = (unsigned long) atoll((myline.substr(0, pos)).c_str());
    hash = std::stoul ((myline.substr(0, pos)).c_str(), nullptr, 0);
    chunk = myline.substr(pos+1, std::string::npos);
    
    for(int i = 0; i < chunk.length(); i++) {
      
      if(chunk[i] == '!')
	chunk[i] = char(13);
      
      if(chunk[i] == '$')
	chunk[i] = char(10);
    }
    chunk_hash.insert(pair<unsigned long, string> (hash, chunk));
    printf ("Read : hash:%lu chunk:%s \n", hash, chunk.c_str());
  }
  
  remove((dump_file).c_str());
  
  return;
}

void * rehydrateFile(void * args) {
  
  dedup_t * rehydrate = (dedup_t *) args;
  
  std::fstream fp;
  fp.open((rehydrate->file_name).c_str(), ios::out | ios::in);				
  if (!fp.is_open()) {
    printf("File:%s is already open!!! \n", (rehydrate->file_name).c_str());
    pthread_exit(NULL);
  }
  
  std::string out_file = rehydrate->file_name + ".dedup";
  std::ofstream ofp;
  ofp.open(out_file, ios::app);
  if (!ofp.is_open()) {
    printf("File:%s is already open!!! \n", out_file.c_str());
    pthread_exit(NULL);
  }
  std::string line;
  while (fp) {
    std::getline(fp, line);
    if (line == "")
      continue;
    
    if (line[0] == '%') {
      line = line.erase(0,1);
      ofp << line;
    } else {
      line = line.substr(1, line.size() - 2);
      unsigned long hash = std::stoul (line.c_str(), nullptr, 0);
      
      std::map<unsigned long, std::string>::iterator pos = chunk_hash.find(hash);
      
      if (pos == chunk_hash.end()) {
	printf ("Chunk not found for has :%lu \n", hash);
      } else {
	std::string chunk = pos->second;
	printf("Thread:%d Found chunk:%s for hash:%lu \n", (int) pthread_self(), chunk.c_str(), hash);
	ofp << chunk;
      }
    }
  }
  
  ofp << "\n";
  fp.close();
  ofp.close();
  remove((rehydrate->file_name).c_str());
  rename(out_file.c_str(), (rehydrate->file_name).c_str());
  
  __sync_fetch_and_sub (&thread_count, 1);
  
  pthread_exit(NULL);
  
  return 0;
  
}

int rehydrateDriver (std::string dir_to_rehydrate, long chunk_size) {

  struct dirent *dir;
  std::string ignore_file = ".dedup";
  static int recursion_count = 0;
  
  if (recursion_count == 0) {
    load_dedup_map(dir_to_rehydrate);
  }
  
  DIR *dp = opendir(dir_to_rehydrate.c_str());
  if(dp) {
    
    while ((dir = readdir(dp)) != NULL) {
      
      string strpath = "";
      string dir_name = dir->d_name;

      //mainStr.compare(mainStr.size() - toMatch.size(), toMatch.size(), toMatch) == 0
      
      if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
	continue;
      
      if (dir->d_type != DT_DIR && 
	  (strcmp(dir->d_name, "dedupe_data.csv") == 0 ||
	   dir_name.compare(dir_name.size() - ignore_file.size(), ignore_file.size(), ignore_file) == 0))
	continue;
            
      if (dir->d_type == DT_DIR) {
	dir_to_rehydrate = checkPath(dir_to_rehydrate);
	strpath = dir_to_rehydrate + "/" + dir->d_name;
	
	recursion_count ++;
	
	if (rehydrateDriver (strpath, chunk_size) != 0)
	  return -1;
	
	continue;
      }
      else {
	dir_to_rehydrate = checkPath(dir_to_rehydrate);
	strpath = dir_to_rehydrate + "/" + dir->d_name;
      }
      
      __sync_synchronize();
      
      while (thread_count >= MAX_THREAD) {
	printf ("Thread count in greater : %d \n", thread_count);
	sleep(1);
      }
      
      rehydrate[thread_count].chunk_size = chunk_size;
      rehydrate[thread_count].file_name = strpath;
      
      pthread_t ptid;
      printf("Creating thread for reading file :%s \n", strpath.c_str());
      
      // Creating a new thread
      if (pthread_create(&ptid, NULL, &rehydrateFile, (void *) &rehydrate[thread_count]) != 0) {
	printf("Error in creating thread..Stopping rehydrate.. \n");
	closedir(dp);
	return -1;
      }
      
      __sync_fetch_and_add (&thread_count, 1);
      pthread_yield();
    }
    
    // wait for thread to finish their work.
    if (recursion_count == 0) {
      while(thread_count > 0 ) {
	printf ("Thread count wait for all thread : %d \n", thread_count);
	sleep(5);
	__sync_synchronize();
      }
    }
    closedir(dp);
  }
  recursion_count --;
  return 0;
}
