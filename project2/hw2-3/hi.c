/* TCPmechod.c - main, echo */
#include "execute.h"
#include "get_command.h"
#include "env.h"
#include "pipe.h"
#include "hi.h"
#include "user.h"
#include "shm.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
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
#include <signal.h>
#include <arpa/inet.h>


#define	QLEN           30	/* maximum connection queue length	*/
#define	BUFSIZE        4096
#define MAXLINE        20000

n_pipe *proot=NULL;
u_pipe *uroot=NULL;
fd_set fds;
//user users[35];
user *userptr;
int *upipetable;
char *message;
int myid;
int usercount;
pid_t pidtable[100];
int message_fd;
pid_t parentpid;
int parentfd;

extern char **environ;
extern int errno;
extern int semid;
extern int shmid;
extern int messagesemid; /* semid for tell2 or yell */
extern int messageshmid; /* shmid for tell2 or yell */
extern int upipeshmid;
extern int upipesemid;
extern int broadsemid;
extern int broadshmid;

int  errexit(const char *format, ...);
int  passiveTCP(char *service, int qlen);
int  echo(int fd);
void process_remote_input(int fifofd,int sockfd,char *command,token *troot);
int readline(int fd,char *ptr , int maxlen);
void err_dump(char *x);
void my_lock(user *userptr);
void my_unlock(user *userptr);
void my_message_lock(char *message);
void my_message_unlock(char *message);
void my_broad_lock();
void my_broad_unlock();
void child_handler(int signo);
void sem_rm(int id);
int sem_open(key_t key);
void sem_close(int id);
void messagehandle(int signo);
void returnhandle(int sogno);

int main(int argc, char *argv[])
{
  //get parent pid
  parentpid = getpid();  
  parentfd = dup(1);

  size_t len = 20000;
  char command[len];
  token *troot;
  struct sockaddr_in cli_addr,serv_addr;
  int sockfd,newsockfd,clilen,childpid;
  usercount=0;

  //initialize path
  char *name="PATH";
  char *value="bin:.";
  cus_setenv(name,value);

  //initialize pid table
  memset(pidtable,0,sizeof(pid_t)*100);

  //new a signal handler
  signal(SIGCHLD,child_handler); 
  //for print message
  signal(SIGUSR1,messagehandle);
  //for return signal
  signal(SIGUSR2,returnhandle);

  //select port
  char *service = "40000";
  switch(argc){
    case 1:
      break;
    case 2:
      service = argv[1];
      break;
  }


  //welcome message
  char *welcome1 ="****************************************\n";
  char *welcome2 ="** Welcome to the information server. **\n";
  char *welcome3 ="****************************************\n";


  //new a socket for listening connection 
  if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0) err_dump("server: can't open stream socket!\n");
  bzero((char *)&serv_addr , sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons((u_short)atoi(service));
  //memset(users,0,sizeof(user)*35);

  //bind socket to a port
  if(bind(sockfd,(struct sockaddr *)&serv_addr , sizeof(serv_addr))<0)
    err_dump("server: can't bind local address!\n");


  //listen for new connection
  listen(sockfd,5);

  for(;;){
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,(struct sockaddr *)&cli_addr , &clilen);
 
    usercount+=1;

    //new shared memory
    if(usercount==1){
      //new a share memory
      if((shmid=shmget(SHMKEY,sizeof(user)*31,PERMS|IPC_CREAT))<0) err_dump("...1");
      if((upipeshmid=shmget(SHMKEY1,sizeof(int)*900,PERMS|IPC_CREAT))<0) err_dump("...2");
      if((messageshmid=shmget(SHMKEY2,sizeof(char)*2000,PERMS|IPC_CREAT))<0) err_dump("...2");

      //attache share memory
      if((userptr=(user*)shmat(shmid,(char *)0,0))==-1) err_dump("...3");
      if((upipetable=(int*)shmat(upipeshmid,(char *)0,0))==-1) err_dump("...4");
      if((message=(char*)shmat(messageshmid,(char *)0,0))==-1) err_dump("...4");

      //initialize users
      memset(userptr,0,sizeof(user)*31);
      memset(upipetable,0,sizeof(int)*900);
      memset(message,0,sizeof(char)*2000);

    }

    if(newsockfd < 0) err_dump("server:accept error");
    if((childpid=fork())<0) err_dump("server:fork error");
    else if(childpid==0){

      //convert newsockfd to global variable
      message_fd = newsockfd;

      char **envp = (char **)malloc(sizeof(char)*100*8);
      memset(envp,0,sizeof(char)*8*100);

      char *envstr ="PATH=bin:.";
      envp[0]=(char *)malloc(strlen(envstr)+5);
      strcpy(envp[0],envstr);
      envp[1]=NULL;
      environ = envp;

      //close fd for listening
      close(sockfd);

      dup2(newsockfd,1);
      dup2(newsockfd,2);

      //fprintf welcome message
      FILE *sockfp = fdopen(newsockfd,"w");
      fprintf(sockfp,"%s",welcome1);
      fprintf(sockfp,"%s",welcome2);
      fprintf(sockfp,"%s",welcome3);
      fflush(sockfp);
  
      //new a user
      //get ip
      struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&cli_addr;
      struct in_addr ipAddr = pV4Addr->sin_addr;
      char str[INET_ADDRSTRLEN];
      inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
      int uindex;

      //write shared memory , so we need to lock it
      my_lock(userptr); 
      for(uindex=0;uindex<30;uindex++){
        if(userptr[uindex].exist==0){
          int strlength = strlen("(no name)");

	  //不能糊塗的用malloc，因為malloc是放在本地端的heap上，而不是shared memory
          //userptr[uindex].ip=(char *)malloc(strlen(str)+5);
	  memset(userptr[uindex].ip,0,30);
	  memset(userptr[uindex].name,0,30);
	  strcpy(userptr[uindex].ip,str);
	  //userptr[uindex].name=(char *)malloc(strlength+5);
	  strcpy(userptr[uindex].name,"(no name)");
	  userptr[uindex].port =(((struct sockaddr_in*)pV4Addr)->sin_port);
	  userptr[uindex].pid  =getpid();
	  userptr[uindex].exist=1;

	  myid = uindex+1;
          break;
	}
      }
      my_unlock(userptr);

      //create fifo for the user
      char fifoname[30];
      memset(fifoname,0,30);
      sprintf(fifoname,"./user_pipe/%d",uindex);
      mknod(fifoname, S_IFIFO | ALLPERMS, 0);
      //有開fifo就沒辦法接收到newsockfd
      //是因為open 沒開權限
      int fifofd = open(fifoname,O_RDWR | O_NONBLOCK);
   
      //broadcast login message
      fprintf(sockfp,"*** User '%s' entered from %s/%d. ***\n",userptr[uindex].name,"CGILAB",511);
      fflush(sockfp);
      
      char loginstr[1050];
      memset(loginstr,0,1050);   
      sprintf(loginstr,"*** User '%s' entered from %s/%d. ***\n",userptr[uindex].name,"CGILAB",511); 
      
      my_message_lock(message);
      strcpy(message,loginstr);
      my_message_unlock(message);

      for(int i=0;i<30;i++){
        if(userptr[i].exist==1 && i!=myid-1){
	  kill(userptr[i].pid,SIGUSR1);
	}
      }

      process_remote_input(fifofd,newsockfd,command,troot);

      exit(0);
    }else{

      //add childpid into pidtable	    
      for(int i=0;i<100;i++){
        if(pidtable[i]==0){
          pidtable[i]=childpid;
	  break;
	}
      }
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
  printf("cc:%d\n",cc);

  if (cc < 0)
    errexit("echo read: %s\n", strerror(errno));
  if (cc && write(1, buf, cc) < 0)
    errexit("echo write: %s\n", strerror(errno));

  write(fd,"% ",2);   

  return cc;
}

void process_remote_input(int fifofd,int sockfd,char *command,token *troot){

  //fdset for  select
  //including fifo and shared memory
  int nfds,fifoflag=0;
  int n;
  FILE *sockfp = fdopen(sockfd,"w");//fprintf to socket


  for(;;){

    write(sockfd,"% ",2);

    n = readline(sockfd,command,MAXLINE);
    if(n==0) return ;
    else if(n<0) err_dump("process_connection:readline error");

    troot = tokenlize(command);
    int n_of_token=0;
    for(token *i=troot;i!=NULL;i=i->next){
      n_of_token=0;
    }
    execute(troot,sockfd,command);

    free_all_token(troot);
    troot=NULL;
    fresh_all_pipe(proot,sockfd);
  }

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

void child_handler(int singo){
  int status;
  pid_t pid;

  while((pid=waitpid(-1,&status,WNOHANG))>0){
    //printf("kill some process!\n");

    for(int i=0;i<100;i++){
      if(pidtable[i]==pid){
        usercount-=1;
	pidtable[i]=0;
	break;
      }
    }
    
    //printf("usercount : %d\n",usercount);
    //detache and delete shared memory and semaphore
    if(usercount==0){
      //printf("detache every thing\n");

      //detache shared memory
      shmdt(userptr);
      shmdt(upipetable); 
      shmdt(message);
  
      //delete shared memory
      shmctl(shmid,IPC_RMID,(struct shmid_ds *)0);
      shmctl(upipeshmid,IPC_RMID,(struct shmid_ds *)0);
      shmctl(messageshmid,IPC_RMID,(struct shmid_ds *)0);

    }
  }
}

void messagehandle(int singo){
  if(parentpid == getpid()){
    //printf("parent!! message\n");
  }else{
    FILE *file = fdopen(message_fd,"w");
    fprintf(file,"%s",message);
    fflush(file);
  }
  /*
  FILE * file = fdopen(parentfd,"w");
  fprintf(file,"send SIGUSR2!! send to pid %d when parent pid == %d\n",userptr[30].pid,parentpid);
  fflush(file);
  */
  //if(userptr[30].pid == 0){

  //}else{
  //kill(userptr[30].pid,SIGUSR2);
  //}
}

void returnhandle(int signo){
  if(parentpid == getpid()){
    //FILE *file = fdopen(parentfd,"w");
    //fprintf(file,"parent!!  return handle\n");
    //fflush(file);
  }
  return;
}

