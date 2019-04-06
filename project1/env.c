#include "env.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern env *en_va;

char *get_a_env(char *name) {
  return getenv(name);
}
void print_a_env(char *name) {
  if(getenv(name)==NULL){
    printf("env %s not found!\n",name);
  }else{
    printf("%s\n",getenv(name));
  }
}
void cus_setenv(char *name,char *value){
  setenv(name,value,1);
}
