#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "env.h"
#include "token.h"
#include "pipe.h"

extern n_pipe *proot;
extern char **environ;


int fork_subroutine(int first_flag,int last_pfd[2],char *file_path,token *tnow,int file_exist_flag){

  int pfd[2];
  int pipe_type=-1;
  char *file_name;
  int number_pipe=0;

  if(tnow->next!=NULL){
    char *next_content = tnow->next->content;
    if(next_content[0]=='|'){
      if(next_content[1]!='\x00'){
        //number normal pipe

	int number = atoi((next_content+1));
	//check for number , if have the same number , don't need to new a node
	//just get the pipe from the number
	//if have no same number , new a new node
        
	int equal_flag=0;
        for(n_pipe *i=proot;i!=NULL;i=i->next){
          if(i->number==number){
            equal_flag=1;
	    pfd[1]=i->fd_write;
            pfd[0]=i->fd_read;
            break;
	  }
	}
        
        if(equal_flag==0){
	  if(pipe(pfd)<0){
            printf("numpipe err!\n");
	  }

          n_pipe *newnode = make_a_pipe(number,pfd[1],pfd[0]);
          newnode->next   = proot;
	  proot           = newnode;

	}	
        pipe_type = 3;
      }else{
        //normal pipe
        if(pipe(pfd)<0){
          printf("pipe error!\n");
	  return 0;
	}
        pipe_type=0;	
      }
    }else if(next_content[0]=='!'){
      if(next_content[1]!='\x00'){
        //number ! pipe
	
	int number = atoi((next_content+1));
	//check for number , if have the same number , don't need to new a node
	//just get the pipe from the number
	//if have no same number , new a new node
        
	int equal_flag=0;
        for(n_pipe *i=proot;i!=NULL;i=i->next){
          if(i->number==number){
            equal_flag=1;
	    pfd[1]=i->fd_write;
            pfd[0]=i->fd_read;
            break;
	  }
	}
        
        if(equal_flag==0){
	  
	  if(pipe(pfd)<0){
            printf("numpipe err!\n");
	  }

          n_pipe *newnode = make_a_pipe(number,pfd[1],pfd[0]);

          newnode->next   = proot;
	  proot           = newnode;

	}	

	pipe_type = 4;
	
      }else{
        //! pipe
	if(pipe(pfd)<0){
          printf("pipe error!\n");
	  return 0;
	}
	pipe_type=1;
      }
    }else if(next_content[0]=='>'){

      file_name = tnow->next->next->content;

      pipe_type=2;
      
    }
  }


  if(first_flag==1){
    //check for numbered pipe

    //if has number pipe , set number_pipe = 1
    //set number_pipe's pfd to last_pfd
    //if has no number pipe , set number_pipe = 0
    for(n_pipe *i=proot;i!=NULL;i=i->next){
      if(i->number==0){
	
        number_pipe=1;	
        last_pfd[1]=i->fd_write;
        last_pfd[0]=i->fd_read;
      
        break;	
      }
    }
  }


  pid_t pid;
  pid_t pid2;

  //printf("start to fork!\n");

  if(pid2=fork()){

    if(first_flag!=1 /*&& tnow->next!=NULL*/ || number_pipe==1){
      number_pipe=0;
      close(last_pfd[0]);
      close(last_pfd[1]);
    }

    if(pipe_type==0 || pipe_type==1){
      last_pfd[0]=pfd[0];
      last_pfd[1]=pfd[1];
    }
    int status;
    pid_t p=waitpid(pid2,&status,0);
    //printf("exit pid:%ld\n",p);

  }else if(!pid2){
    //child

    //printf("enter exit\n");
    if(pid=fork()){
      //parent
      //為了能讓%顯示在最下面，最後一個指令要等到結束
      //numbered pipe 會出問題的樣子
      
      if(first_flag != 1 || number_pipe==1){ 
        number_pipe=0;
        close(last_pfd[0]);
        close(last_pfd[1]);
      }
      
      if(tnow->next==NULL || pipe_type==2 /*|| pipe_type==4 || pipe_type==3*/){
	int status;
        pid_t p=waitpid(pid,&status,0);
	//printf("pid : %ld\n",p);
      }

      exit(1);

    }else if(!pid){
      //child

      //前面有pipe
      //或是有numbered pipe
      if(first_flag!=1 || number_pipe==1){
        close(last_pfd[1]);
        dup2(last_pfd[0],0);
        close(last_pfd[0]);
      }

      //後面有pipe
      if(pipe_type==0 || pipe_type==3){
	//normal pipe
	//normal numbered pipe
	
        close(pfd[0]);
        dup2(pfd[1],1);
        close(pfd[1]); 
      }else if(pipe_type==1 || pipe_type ==4){
        //! pipe
	//! numbered pipe

        close(pfd[0]);
	dup2(pfd[1],1);//stdout
	dup2(pfd[1],2);//stderr
        close(pfd[1]);
      }else if(pipe_type==2){
	//> redirection
	remove(file_name);
        int out = open(file_name, O_RDWR|O_CREAT, 0600);
        dup2(out,fileno(stdout));
        close(out);	
      }

      //set argv
      int n_parameter=0;
      for(token *i=tnow->parameters;i!=NULL;i=i->next){
        n_parameter++;
      }

      char *argv[n_parameter+2];
      argv[0]=file_path;
      int index=1;
      for(token *i=tnow->parameters;i!=NULL;i=i->next){
        argv[index++]=i->content;
      }
      argv[index]=NULL;
      //set env
      char **envp=environ;

      if(file_exist_flag==1){
        execve(argv[0],argv,envp);    
        exit(0);
      }else{
        //fprintf(stderr,"Unknown command: [%s].\n",tnow->content);
        fprintf(stderr,"Unknown command: [%s].\n",tnow->content);
	exit(0);
      }


    }else{
      fprintf(stderr,"fork error!\n");
      //if fork error occur , just fork again
    
      close(pfd[0]);
      close(pfd[1]);
      //可能會有pipe成功開起來，但fork壞掉的情況    
      //這時候要把pipe直接關掉
    
    
      return 0;
    }
  }else{
    fprintf(stderr,"fork error!!\n");
  }

  //numbered pipe 已經用掉了，所以可以free掉
  free_0_pipe(&proot);

  
  return 1;
}


void execute(token *troot){
  token *tnow = troot;
  token *tlast= NULL;
  int last_pfd[2];
  int first_flag=1;

  //look back to see whether '>' exist
  for(token *i=troot;i!=NULL;i=i->next){
    if(i->content[0]=='>'){
      int out = open(i->next->content, O_RDWR|O_CREAT, 0600);
      close(out);
    }
  }


  while (tnow!=NULL) {
    if(strcmp(tnow->content,"exit")==0){

      exit(0);

    }else if(strcmp(tnow->content,"setenv")==0){
      char *name = tnow->parameters->content;
      char *value= tnow->parameters->next->content;
      cus_setenv(name,value);

    }else if(strcmp(tnow->content,"printenv")==0){

      char *name = tnow->parameters->content;
      print_a_env(name);

    }else if(tnow->content[0]=='|'){

    }else if(tnow->content[0]=='>'){
      tnow->next=NULL;//結束執行
    }else if(tnow->content[0]=='!') {

    }else {
      //get path
      char *path_from_env=get_a_env("PATH");
      char *path = (char *)malloc(strlen(path_from_env)+10);
      strcpy(path,path_from_env);

      char *file_name=tnow->content;

      int path_len=strlen(path);
      int num_path=0;
      for(int i=0;i<path_len;i++){
        if(path[i]==':'){
          num_path++;
	}
      }
      num_path++;

      char *paths[num_path];
      int position[num_path];
      int pindex=0;
      position[pindex++]=0;

      //bin:. --> "bin" + '\x00' + '.' + '\x00'
      //to make strcpy easily 
      for(int i=0;i<path_len;i++){
        if(path[i]==':'){
          path[i]='\x00';
          position[pindex++]=i+1;
	}
      }
      for(int i=0;i<num_path;i++){
        paths[i] = (char *)malloc(strlen(path+position[i])+strlen(file_name)+10);
	strcpy(paths[i],path+position[i]);
	int str_len = strlen(paths[i]);
	paths[i][str_len]='/';
	paths[i][str_len+1]='\x00';
	strcat(paths[i],file_name);
      }

      char *file_path;

      int file_exist_flag=0;
      for(int i=0;i<num_path;i++){
        if(access(paths[i],F_OK)!=-1){
          //file exist
	  file_exist_flag=1;
	  file_path = paths[i];
	  break;
	}
      }
      //end get path

      //check whether the file exist
      
      /*
      if(file_exist_flag==0){
	//file doesn't exist
        printf("Unknown command: [%s].\n",file_name);
      }else{
	//file exist

	//if fork or pipe error , just pipe or fork again 
	while(!fork_subroutine(first_flag,last_pfd,file_path,tnow)); 
  
	if(first_flag==1){
          first_flag=0;
	}
      }*/
      //if fork or pipe error , just pipe or fork again 
      while(!fork_subroutine(first_flag,last_pfd,file_path,tnow,file_exist_flag)); 
  
      if(first_flag==1){
        first_flag=0;
      }

      
      for(int i=0;i<num_path;i++){
        free(paths[i]);
	//file_path is here , so don't need to free again
      }

      free(path);
    }
    if(tnow!=NULL){
      tnow=tnow->next;
    }
  }
  int status;
  //wait for all child process 
  //if numbered pipe , then don't need to wait 
  waitpid(0,&status,0);
  //free all the tnow
  
}

