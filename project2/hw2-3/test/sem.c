#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>


int main(){

  int id;
  int key=555;

  id = semget(key, 3, 0666 | IPC_CREAT);



  return 0;
}
