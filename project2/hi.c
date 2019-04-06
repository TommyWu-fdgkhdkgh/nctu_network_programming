#include "execute.h"
#include "get_command.h"
#include "env.h"
#include "pipe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define MAXLINE 20000

n_pipe *proot=NULL;
extern char **environ;

void err_dump(const char *x);
void process_connection(int sockfd,char *command,token *troot);
int readline(int fd,char *ptr , int maxlen);
void child_handler(int signo);

int main(int argc, char **argv, char **envp) {
  int sockfd,newsockfd,clilen,childpid;
  struct sockaddr_in cli_addr , serv_addr;

  size_t len = 20000;
  char *service = "40000";
  char command[len];
  char in;
  int index;
  token *troot;

  //new a signal handler
  signal(SIGCHLD,child_handler);

  //set PATH
  char *name = "PATH";
  char *value= "bin:.";
  cus_setenv(name,value);

  switch (argc){
    case 1:
      break;
    case 2:
      service = argv[1];
      break;
  } 


  //new a socket for listening new connection
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) err_dump("server: can't open stream socket"); 
  bzero((char *)&serv_addr , sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons((u_short)atoi(service));

  //bind socket to port
  int on=1;
  if((setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0){
    err_dump("gg\n");
  }
  

  if(bind(sockfd,(struct sockaddr *)&serv_addr , sizeof(serv_addr))<0)
    err_dump("server: can't bind local address!\n");

  listen(sockfd,5);

  for(;;){
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd,(struct sockaddr *)&cli_addr , &clilen);

    if(newsockfd < 0) err_dump("server:accept error");
    if((childpid=fork())<0) err_dump("server:fork error");
    else if(childpid==0){

      char **envp = (char **)malloc(sizeof(char)*100*8);
      memset(envp,0,sizeof(char)*8*100);

      char *envstr ="PATH=bin:.";
      envp[0]=(char *)malloc(strlen(envstr)+5);
      strcpy(envp[0],envstr);
      envp[1]=NULL;
      environ = envp;


      close(sockfd);

      dup2(newsockfd,1); 
      dup2(newsockfd,2);

      process_connection(newsockfd,command,troot);
      exit(0);
    }
    close(newsockfd);
  }


  return 0;
}

void process_connection(int sockfd,char *command,token *troot){

  int n;

  for(;;){

    fflush(stdout);
    printf("%% ");
    fflush(stdout);

    n = readline(sockfd,command,MAXLINE);
    if(n==0) return ;
    else if(n<0) err_dump("process_connection:readline error");

    troot = tokenlize(command);
    int n_of_token=0;
    for(token *i=troot;i!=NULL;i=i->next){
      n_of_token=0;
    }
    execute(troot);

    free_all_token(troot);
    troot=NULL;
    fresh_all_pipe(proot);
  }
}

void err_dump(const char *x){
  perror(x);
  exit(1);
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

void child_handler(int singo){
  int status;
  while(waitpid(-1,&status,WNOHANG)>0){
    //printf("kill some process!\n");
  }
}

