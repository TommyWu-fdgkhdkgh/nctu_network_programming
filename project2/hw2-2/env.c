#include "env.h"
#include "user.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


extern user users[35];
extern env *en_va;

int  get_a_env(char *name,int myid,char *returnstr) {
  for(int i=0;users[myid-1].envp[i]!=NULL;i++){
    char *n;
    char *v;

    char str[100];
    memset(str,0,100);
    strcpy(str,users[myid-1].envp[i]);

     
    n=strtok(str,"=");
    v=strtok(NULL,"="); 

    //strtok 會把現有的字串竄改掉

    if(strcmp(n,name)==0){
      strcpy(returnstr,v); 
      return 1;
    }
  }
  return 0;
}
void print_a_env(int fd,char *name,int myid) {

  FILE *file = fdopen(fd,"w");

  char *n;
  char *v;
  char str[100];

  for(int i=0;users[myid-1].envp[i]!=NULL;i++){
    memset(str,0,100);
    strcpy(str,users[myid-1].envp[i]);
    n=strtok(str,"=");
    v=strtok(NULL,"=");

    if(strcmp(n,name)==0){
      fprintf(file,"%s\n",v);
    }
  }  

  fflush(file);
}
void cus_setenv(char *name,char *value,int myid){

  //setenv(name,value,1);
  //just change the path stored in user structure
  int index,find_flag=0;
  char str[100];
  memset(str,0,100);
  char *n;
  char *v;

  for(index=0;users[myid-1].envp[index]!=NULL;index++){

    strcpy(str,users[myid-1].envp[index]);    
    
    n = strtok(str,"=");
    v = strtok(NULL,"=");

    if(strcmp(n,name)==0){
      free(users[myid-1].envp[index]);
      
      char str[100];
      memset(str,0,100);
      sprintf(str,"%s=%s",name,value);

      users[myid-1].envp[index] = (char *)malloc(strlen(str)+4);
      strcpy(users[myid-1].envp[index],str);

      find_flag=1;

      break;
    }
  }
  
  //insert a new envp
  if(find_flag==0){
    char str[100];
    memset(str,0,100);
    sprintf(str,"%s=%s",name,value);

    users[myid-1].envp[index] = (char *)malloc(strlen(str)+4);
    strcpy(users[myid-1].envp[index],str);
  }
}
