#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user.h"


void free_user(user *users,int id){

  free(users[id-1].name);
  free(users[id-1].ip);
  users[id-1].exist=0;

}

