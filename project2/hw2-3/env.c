#include "env.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern env *en_va;

char *get_a_env(char *name) {
  return getenv(name);
}
void print_a_env(int fd,char *name) {
  FILE *file = fdopen(fd,"w");

  if(getenv(name)==NULL){
    fprintf(file,"env %s not found!\n",name);
  }else{
    fprintf(file,"%s\n",getenv(name));
  }
  fflush(file);
}
void cus_setenv(char *name,char *value){
  setenv(name,value,1);
}
