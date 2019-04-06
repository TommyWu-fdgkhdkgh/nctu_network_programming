#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>


#define QLEN 3


int errexit(const char *format , ...);
int passiveTCP(char *service,int qlen );
void reaper(int signo);
int TCPechod(int fd);
int passivesock(char *service , char *protocol , int qlen);

int main(int argc ,char *argv[]){
  
  char *service = "echo";//"echo"; /* service name or port number */
  struct sockaddr_in fsin; /* the address of a client */
  int alen;
  int msock; 
  int ssock; 
  
  switch (argc) { 
    case 1:
      break; 
    case 2:
      service = argv[1];
      break; 
    default:
      errexit("usage: TCPechod [port]\n"); 
  }
  msock = passiveTCP(service, QLEN);
  (void) signal(SIGCHLD, reaper);

  while (1) {
    alen = sizeof(fsin);
    ssock = accept(msock, (struct sockaddr *) &fsin, &alen);
    if (ssock < 0) {
      if (errno == EINTR)
        continue;
      errexit("accept: %s\n", sys_errlist[errno]);
    }
    switch(fork()){
      case 0: /*child*/
        (void)close(msock);
        exit(TCPechod(ssock));       
      default:/*parent*/
        (void)close(ssock); 
        break;
      case -1:
	errexit("fork %s\n",sys_errlist[errno]);
    }
  }

  return 0;
}


int TCPechod(int fd) 
{
  char buf[BUFSIZ]; 
  
  int cc;

  while (cc = read(fd, buf, sizeof(buf))){ 
    if (cc < 0)
      errexit("echo read: %s\n", sys_errlist[errno]);
    if (write(fd, buf, cc) < 0)
      errexit("echo write: %s\n", sys_errlist[errno]);
  }

  return 0; 
}


void reaper(int signo){
  int status;
  while (wait3(&status, WNOHANG,(struct rusage *)0)>= 0)
  /* empty */; 
}

int
errexit(const char *format, ...)
{
	va_list	args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(1);
}

/* passiveTCP.c - passiveTCP */

/*------------------------------------------------------------------------
 * passiveTCP - create a passive socket for use in a TCP server
 *------------------------------------------------------------------------
 */
int passiveTCP( service, qlen )
char	*service;	/* service associated with the desired port	*/
int	qlen;		/* maximum server request queue length		*/
{
	return passivesock(service, "tcp", qlen);
}

int passiveUDP( service )
char *service; /* service associated with the desired port */ 
{
  return passivesock(service, "udp", 0); 
}

int passivesock(char *service , char *protocol , int qlen){
  struct servent *pse;
  struct protoent *ppe;
  struct sockaddr_in sin;
  int s, type; /* socket descriptor and socket type */
  bzero((char *)&sin, sizeof(sin)); 
  sin.sin_family = AF_INET; 
  sin.sin_addr.s_addr = INADDR_ANY;


  if ( pse = getservbyname(service, protocol) )
    sin.sin_port = htons(ntohs((u_short)pse->s_port) /*+ portbase*/); 
  else if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 )
    errexit("can't get \"%s\" service entry\n", service);
  /* Map protocol name to protocol number */ 
  if ( (ppe = getprotobyname(protocol)) == 0)
    errexit("can't get \"%s\" protocol entry\n", protocol);
  /* Use protocol to choose a socket type */ 
  if (strcmp(protocol, "udp") == 0)
    type = SOCK_DGRAM; 
  else
    type = SOCK_STREAM;


  /* Allocate a socket */
  s = socket(PF_INET, type, ppe->p_proto); 
  if (s < 0)
    errexit("can't create socket: %s\n", sys_errlist[errno]);
  /* Bind the socket */
  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    errexit("can't bind to %s port: %s\n", service, sys_errlist[errno]);
  if (type == SOCK_STREAM && listen(s, qlen) < 0) 
    errexit("can't listen on %s port: %s\n", service,sys_errlist[errno]);

  return s; 
}


