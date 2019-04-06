/* TCPmechod.c - main, echo */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>


#define	QLEN		  30	/* maximum connection queue length	*/
#define	BUFSIZE		4096

typedef struct USER{
  int fd;
  char *name;
  int exist; 
}user;


extern int	errno;
int		errexit(const char *format, ...);
int		passiveTCP(char *service, int qlen);
int		echo(int fd);

/*------------------------------------------------------------------------
 * main - Concurrent TCP server for ECHO service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
  char *welcome1 ="***************************************\n";
  char *welcome2 ="** Welcome to the information server **\n";
  char *welcome3 ="***************************************\n";

  user users[35];
  memset(users,0,sizeof(user)*35);

  char *service = "40000";	/* service name or port number	*/
  struct sockaddr_in fsin;	/* the from address of a client	*/
  int msock;			/* master server socket		*/
  fd_set rfds;			/* read file descriptor set	*/
  fd_set afds;			/* active file descriptor set	*/
  unsigned int alen;		/* from-address length		*/
  int fd, nfds;
	
  switch (argc) {
  case	1:
    break;
  case	2:
    service = argv[1];
    break;
  default:
    errexit("usage: TCPmechod [port]\n");
  }

  msock = passiveTCP(service, QLEN);

  nfds = getdtablesize();
  FD_ZERO(&afds);
  FD_SET(msock, &afds);

  while (1) {
    //拷貝afds到rfds
    //把現在正在連接的連線們放到rfds
    memcpy(&rfds, &afds, sizeof(rfds));

    //只要有接收完成的，就放到rfds裡面
    //處理舊有使用者的輸入
    if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0,
       (struct timeval *)0) < 0)
      errexit("select: %s\n", strerror(errno));

    //新的使用者
    if (FD_ISSET(msock, &rfds)) {
      int ssock;

      alen = sizeof(fsin);
      ssock = accept(msock, (struct sockaddr *)&fsin,&alen);

      //get ip
      struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&fsin;
      struct in_addr ipAddr = pV4Addr->sin_addr;
      char str[INET_ADDRSTRLEN];
      inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
      printf("ip:%s\n",str);

      //get port
      //不知道為什麼，port會亂跳，而不是telnet的port
      printf("%d\n",(((struct sockaddr_in*)pV4Addr)->sin_port));
		        	

      if (ssock < 0)
        errexit("accept: %s\n",strerror(errno));

      FD_SET(ssock, &afds);
                       
      //welcome message
      write(ssock,welcome1,strlen(welcome1));
      write(ssock,welcome2,strlen(welcome2));
      write(ssock,welcome3,strlen(welcome3));
      //add user
      int user_index;
      for(user_index=0;user_index<35;user_index++){
        if(users[user_index].exist==0){
          break;
        } 
      }
      //initialize user
      int strlength = strlen("(no name)");
      users[user_index].exist=1;
      users[user_index].fd   =ssock;
      users[user_index].name =(char *)malloc(sizeof(char)*strlength+10);
      strcpy(users[user_index].name,"(no name)");
			
      //broad cast login message
      for(int i=0;i<35;i++){
        if(users[i].exist==1){
          FILE *file = fdopen(users[i].fd,"w");
          fprintf(file,"*** User ’%s’ entered from %s/%d. ***\n",users[user_index].name,str, (((struct sockaddr_in*)pV4Addr)->sin_port));
          fflush(file);
	}
      }

    }
    //遍尋每一個使用者，假如使用者有輸入東西，則處理
    for (fd=0; fd<nfds; ++fd)
      if (fd != msock && FD_ISSET(fd, &rfds))
        //if (fd != msock && FD_ISSET(fd, &afds))
        //假如印出0個字元，則關閉連線。
        if (echo(fd) == 3) {

          FILE *file=fdopen(fd,"w");
          fprintf(file,"hihi\n");
          fflush(file);
	    
          (void) close(fd);
          FD_CLR(fd, &afds);
        }
  }
}

/*------------------------------------------------------------------------
 * echo - echo one buffer of data, returning byte count
 *------------------------------------------------------------------------
 */
int
echo(int fd)
{
  char	buf[BUFSIZ];
  int	cc;

  cc = read(fd, buf, sizeof buf);
  if (cc < 0)
    errexit("echo read: %s\n", strerror(errno));
  if (cc && write(fd, buf, cc) < 0)
    errexit("echo write: %s\n", strerror(errno));
  return cc;
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
  //BOOL bReuseaddr=TRUE;
  int reuse = 1;
  setsockopt(s,SOL_SOCKET ,SO_REUSEADDR,(const char*)&reuse,sizeof(reuse));
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
int passiveTCP( service, qlen )
char    *service;       /* service associated with the desired port     */
int     qlen;           /* maximum server request queue length          */
{
        return passivesock(service, "tcp", qlen);
}

int errexit(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  exit(1);
}
