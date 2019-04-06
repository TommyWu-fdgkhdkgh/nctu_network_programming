#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


#define MAXBUFF 1000
#define FIFO1 "./fifo.1"
#define FIFO2 "./fifo.2"

void client(int readfd,int writefd);

int main(){

  int readfd, writefd;
  // Open the FIFOs. We assume the server has already created them.
  if ( (writefd = open(FIFO1, 1)) < 0)
    fprintf(stderr,"client: can't open write fifo: %s", FIFO1);
  if ( (readfd = open(FIFO2, 0)) < 0)
    fprintf(stderr,"client: can't open read fifo: %s", FIFO2);
  client(readfd, writefd);
  close(readfd);
  close(writefd);
  // Delete the FIFOs, now that we're finished.
  if (unlink(FIFO1) < 0) fprintf(stderr,"client: can't unlink %s", FIFO1); 
  if (unlink(FIFO2) < 0) fprintf(stderr,"client: can't unlink %s", FIFO2);

  return 0;
}

void client(int readfd,int writefd){
  char buff[MAXBUFF]; 
  int n;

  // Read the filename from standard input, write it to the IPC descriptor.
  if (fgets(buff, MAXBUFF, stdin) == NULL) 
    fprintf(stderr,"client: filename read error");

  n = strlen(buff);

  if (buff[n-1] == '\n') n--; /* ignore newline from fgets() */

  if (write(writefd, buff, n) != n) fprintf(stderr,"client: filename write error");

  // Read the data from the IPC descriptor and write to standard output. 

  while ( (n = read(readfd, buff, MAXBUFF)) > 0)
    if (write(1 /* stdout*/, buff, n) != n) fprintf(stderr,"client: data write error"); 
  if (n < 0) fprintf(stderr,"client: data read error");

}




