#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define QLEN 5
#define BUFSIZE 4096

extern int errno;
int errexit(const char *format, ...);
int passiveTCP(const char *service,int qlen);
int echo(int fd);


int main(){

  char *service = "40000";
  struct sockaddr_in fsin;
  int msock;
  fd_set rfds;
  fd_set afds;

  int alen;
  int fd,nfds;

  switch(args){
    case 1:
      break;
    case 2:
      service = argv[1];
      break;
    default:
      errexit("usage:TCPmechod [port]\n");
  }

  msock = passiveTCP(service,QLEN);
  nfds = getdtablesize();
  FD_ZERO(&afds);
  FD_SET(msock,&afds);

  while(1){
    memcpy(&rfds,&afds,sizeof(rfds));




  }

  return 0;
}
