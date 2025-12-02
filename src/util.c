#include <util.h>

#ifdef __linux__
void unix_error(char *msg){
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}
#endif

void* S_malloc(size_t size){
  void* res = malloc(size);

  if(!res){
    unix_error("memory allocation failed.");
    exit(1);
  }
  
  return res;
}
