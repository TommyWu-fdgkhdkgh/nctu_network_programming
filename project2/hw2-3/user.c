#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user.h"


void free_user(user *userptr,int id){

  //free(userptr[id-1].name);
  //free(userptr[id-1].ip);
  userptr[id-1].exist=0;

}

