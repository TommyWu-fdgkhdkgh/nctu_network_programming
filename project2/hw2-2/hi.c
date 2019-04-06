/* TCPmechod.c - main, echo */
#include "execute.h"
#include "get_command.h"
#include "env.h"
#include "pipe.h"
#include "hi.h"
#include "user.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>


#define	QLEN           30	/* maximum connection queue length	*/
#define	BUFSIZE        4096
#define MAXLINE        20000

n_pipe *proot=NULL;
u_pipe *uroot=NULL;
fd_set rfds;			/* read file descriptor set	*/
fd_set afds;			/* active file descriptor set	*/
int myid;

extern char **environ;          /*environment variable*/

extern int errno;
int errexit(const char *format, ...);
int passiveTCP(char *service, int qlen);
int echo(int fd);
void process_remote_input(int fd,char *command,token *troot,int myid);
int readline(int fd,char *ptr , int maxlen);
void err_dump(char *x);
user users[35];
char **environ_backup;          /*back up pointer to environment variable*/

int main(int argc, char *argv[])
{
  size_t len = 20000;
  char command[len];
  char in;
  int index;
  token *troot;

  //initialize environment variable
  /*for(int i=0;environ[i]!=NULL;i++){
    char *n;
    char *v;
    n = strtok(environ[i],"=");
    v = strtok(NULL,"=");
        
    unsetenv(n); 
  }*/

  char *name="PATH";
  char *value="bin:.";
  setenv(name,value,1);

  //backup environment variable
  environ_backup = (char **)malloc(sizeof(char *)*100);
  memset(environ_backup,0,sizeof(char*)*100);
  for(int i=0;environ[i]!=NULL;i++){
    environ_backup[i]=(char *)malloc(strlen(environ[i])+5);
    strcpy(environ_backup[i],environ[i]);
  }


  //welcome message
  char *welcome1 ="****************************************\n";
  char *welcome2 ="** Welcome to the information server. **\n";
  char *welcome3 ="****************************************\n";

  memset(users,0,sizeof(user)*35);

  char *service = "40000";	/* service name or port number	*/
  struct sockaddr_in fsin;	/* the from address of a client	*/
  int msock;			/* master server socket		*/
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

  //bind
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
      //printf("ip:%s\n",str);

      //get port
      //不知道為什麼，port會亂跳，而不是telnet的port
      //printf("%d\n",(((struct sockaddr_in*)pV4Addr)->sin_port));
      

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
      users[user_index].port =(((struct sockaddr_in*)pV4Addr)->sin_port);
      users[user_index].ip   =(char *)malloc(strlen(str)+10);
      strcpy(users[user_index].ip,str);
      strcpy(users[user_index].name,"(no name)");

      //memset(users[user_index].envp,0,sizeof(char *)*100);
      for(int i=0 ;i<100;i++){
        users[user_index].envp[i]=NULL;
      }

      for(int i=0;environ_backup[i]!=NULL;i++){
        char *n;
        n = strtok(environ[i],"=");
   
	if(strcmp(n,"PATH")==0){
          users[user_index].envp[0] = (char *)malloc(strlen(environ_backup[i])+5);
          strcpy(users[user_index].envp[0],environ_backup[i]);
	  break;
	}
      }

      //broad cast login message
      for(int i=0;i<35;i++){
        if(users[i].exist==1){
          FILE *file = fdopen(users[i].fd,"w");
          //fprintf(file,"*** User '%s' entered from %s/%d. ***\n",users[user_index].name,str, (((struct sockaddr_in*)pV4Addr)->sin_port));
          fprintf(file,"*** User '%s' entered from %s/%d. ***\n",users[user_index].name,"CGILAB",511);
          fflush(file);
	}
      }
      
      write(ssock,"% ",2);
          
    }
    //遍尋每一個使用者，假如使用者有輸入東西，則處理
    for (fd=0; fd<nfds; ++fd)
      if (fd != msock && FD_ISSET(fd, &rfds)){
        //if (fd != msock && FD_ISSET(fd, &afds))
        //假如印出0個字元，則關閉連線。

	int uid;
	for(uid=0;uid<35;uid++){
          if(users[uid].fd==fd){
            break;
	  } 
	}

	myid = uid+1;

	//1.clear all the environ
	//2.set all the user's environment variable to environ
	//3.process
	//4.store data from environ to user 
	//5.restore environ_backup to environ

        process_remote_input(fd,command,troot,myid);
      }  
  }
}

int readline(int fd,char *ptr , int maxlen){
  int n , rc;
  char c;
  for(n=1;n<maxlen;n++){
    if((rc=read(fd,&c,1))==1){
       if(c==13) continue;
       if(c=='\n' || c=='\r') break;
       *ptr++=c;
    }else if(rc==0){
      if(n==1)
        return 0;
      else
        break;
    }else{

      return -1;

    }
  }

  *ptr = 0;
  return n;
}


/*------------------------------------------------------------------------
 * echo - echo one buffer of data, returning byte count
 *------------------------------------------------------------------------
 */
int echo(int fd)
{
  char	buf[BUFSIZ];
  int	cc;

  cc = read(fd, buf, sizeof buf);
 
  for(int i=0;i<cc;i++){
    printf("%d\n",buf[i]); 
  }

  if (cc < 0)
    errexit("echo read: %s\n", strerror(errno));
  if (cc && write(fd, buf, cc) < 0)
    errexit("echo write: %s\n", strerror(errno));

  write(fd,"% ",2);   

  return cc;
}

void process_remote_input(int fd,char *command,token *troot,int myid){

  int n;

  //let the result of execution redirect to child process
  //dup2(fd,1);
  //dup2(fd,2);

  n = readline(fd,command,MAXLINE);

  if(n==0) return;
  else if(n<0) err_dump("process_connection:readline error");

  troot = tokenlize(command);
  int n_of_token=0;
  for(token *i=troot;i!=NULL;i=i->next){
    n_of_token=0;
  }
  execute(troot,fd,command,myid);

  /*FILE *file = fdopen(fd,"w");
  fprintf(file,"%% ");
  fflush(file);*/
  write(fd,"% ",2);

  free_all_token(troot);
  troot=NULL;
  fresh_all_pipe(proot,fd);
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
  s = socket(PF_INET, type, ppe->p_proto);
  setsockopt(s,SOL_SOCKET ,SO_REUSEADDR,&reuse,sizeof(reuse));
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
void err_dump(char *x){
  perror(x);
  exit(1);
}

int errexit(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  exit(1);
}
