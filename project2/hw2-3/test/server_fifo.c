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


void server(int readfd,int writefd);

extern int errno;

int main(){
  int readfd, writefd;
  // Create the FIFOs, then open them - one for reading and one for writing. 
  if ( (mknod(FIFO1, S_IFIFO | ALLPERMS, 0) < 0) && (errno != EEXIST))
    fprintf(stderr,"can't create fifo: %s", FIFO1);
  if ( (mknod(FIFO2, S_IFIFO | ALLPERMS, 0) < 0) && (errno != EEXIST)) {
    unlink(FIFO1);
    fprintf(stderr,"can't create fifo: %s", FIFO2);
  }
  if ( (readfd = open(FIFO1, 0)) < 0)
    fprintf(stderr,"server: can't open read fifo: %s", FIFO1);
  if ( (writefd = open(FIFO2, 1)) < 0)
    fprintf(stderr,"server: can't open write fifo: %s", FIFO2);
  server(readfd, writefd);
  close(readfd);
  close(writefd);
  return 0;
}


void server(int readfd, int writefd)
{
  char buff[MAXBUFF], errmesg[256]; //, *sys_err_str(); 
  int n, fd;
  // Read the filename from the IPC descriptor.
  if ( (n = read(readfd, buff, MAXBUFF)) <= 0) 
    fprintf(stderr,"server: filename read error"); 
  buff[n] = '\0'; /* null terminate filename */
  if ( (fd = open(buff, 0)) < 0) {
    // Error. Format an error message and send it back to the client. 
    //sprintf(errmesg, ": can't open, %s\n", sys_err_str());
    strcat(buff, errmesg);
    n = strlen(buff);
    if (write(writefd, buff, n) != n) fprintf(stderr,"server: errmesg write error");
  } else {
    // Read the data from the file and write to the IPC descriptor. 
    while ( (n = read(fd, buff, MAXBUFF)) > 0)
      if (write(writefd, buff, n) != n) fprintf(stderr,"server: data write error"); 
    if (n < 0) fprintf(stderr,"server: read error");
  } 
}




