#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include "shm.h"

int main() {
  // Get the shared memory segment and attach it.
  // The server must have already created it.
  if ( (shmid = shmget(SHMKEY, sizeof(Mesg), 0)) < 0) printf("...");
  if ( (mesgptr = (Mesg *) shmat(shmid, (char *) 0, 0)) == -1) printf("..."); // Open the two semaphores. The server must have created them already.
  if ( (clisem = sem_open(SEMKEY1)) < 0) printf("...");
  if ( (servsem = sem_open(SEMKEY2)) < 0) printf("...");
  client();
  // Detach and remove the shared memory segment and close the semaphores.
  if (shmdt(mesgptr) < 0) printf("...");
  if (shmctl(shmid, IPC_RMID, (struct shmid_ds *) 0) < 0) printf("...");

  sem_close(clisem); 
  sem_close(servsem); 
  exit(0);
}
/* will remove the semaphore */ /* will remove the semaphore */
