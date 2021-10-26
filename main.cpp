#include "common.hpp"


unsigned long sizeof_dir(std:: string str_Path) {
  
  unsigned long file_size = 0;
  struct dirent *dir;
  std::fstream file1;
  DIR *dp = opendir(str_Path.c_str());
  if (dp) {
    while ((dir = readdir(dp)) != NULL) {
      string strpath = "";
      if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
	continue;
      if (dir->d_type == DT_DIR) {
	  str_Path = checkPath(str_Path);
	  strpath = str_Path + "/" + dir->d_name;
	  file_size = file_size + sizeof_dir(strpath);
	  continue;
      }
      else {
	str_Path = checkPath(str_Path);
	strpath = str_Path + "/" + dir->d_name;
      }
      file1.open(strpath.c_str(), ios::out | ios::in | ios::binary);
      file1.seekg(0, ios::end);
      file_size = file_size+ file1.tellg();
      file1.close();
    }
    closedir(dp);
  }
  return file_size;
}


int main(int argc, char *argv[]) {
  clock_t tStart = clock();
  int chunk_size = 0;
  
  if (argc <= 3) {
    printf ("Usage : %s -d|-r <dir-to-dedupe> <chunk_size> \n", argv[0]);
    printf ("-d - To dedupe a directory \n");
    printf ("-r - To rehydrate a directory \n");
    exit(1);
  }
  
  chunk_size = atoi(argv[3]);
  
  if (chunk_size <= 16) {
    printf("Using default chunk size"); 
    chunk_size = 32;
  }
  printf( "using chunk size: %d \n", chunk_size);
  
  std::string dir = argv[2];
  std::string operation = argv[1];
  unsigned long dir_size_before_dedup = sizeof_dir(dir);
  
  if (operation == "-d" || operation == "--dedup") {
    
    printf("Before deduplication %lu kb \n", dir_size_before_dedup);
    printf("Starting Dedup \n");
    printf("Please Wait !!!! \n");
    
    // Start dedup on the directory
    if (dedupeDriver (dir, chunk_size) != 0) {
      printf("Error in dedupe!!! \n");
      exit (1);
    }
    
    unsigned long dir_size_after_dedup = sizeof_dir(dir);
    printf("After deduplication %lu kb \n", dir_size_after_dedup);
    float dedup_per =  (dir_size_after_dedup / dir_size_before_dedup) ;
    float  dedup_ratio = (dedup_per * 100);
    printf("dedup ratio is %.2f \n", dedup_ratio);
    printf("Dedup finish \n");
    printf("Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
    
  } else if (operation == "-r" || operation == "--rehydrate") {
    
    printf("Before rehydration %lu kb \n", dir_size_before_dedup);
    printf("Starting Rehydrate \n");
    printf("Please Wait !!!! \n");
    
    // Start rehydrate on the directory
    if (rehydrateDriver (dir, chunk_size) != 0) {
      printf("Error in rehydrate!!! \n");
      exit (1);
    }
    
    unsigned long dir_size_after_dedup = sizeof_dir(dir);
    printf("After rehydration %lu kb \n", dir_size_after_dedup);
    printf("Rehydrate finish \n");
    printf("Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);

  } else {
    printf ("Usage : %s -d|-r <dir-to-dedupe> <chunk_size> \n", argv[0]);
    printf ("-d - To dedupe a directory \n");
    printf ("-r - To rehydrate a directory \n");
  }
  return 0;
}
