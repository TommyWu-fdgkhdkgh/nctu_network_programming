#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "env.h"
#include "token.h"
#include "pipe.h"
#include "hi.h"
#include "user.h"

extern n_pipe *proot;
extern u_pipe *uroot;
extern char **environ;
extern user users[35];
extern fd_set rfds;
extern fd_set afds;

int fork_subroutine(int first_flag,int last_pfd[2],char *file_path,token *tnow,int file_exist_flag,int fd,char *command){

  int pfd[2];
  int pipe_type=-1;
  char *file_name;
  int number_pipe=0;
  int user_pipe_ocurr=0;
  int myid;

  myid=0;
  for(myid=0;myid<35;myid++){
    if(users[myid].fd==fd){ 
      break;
    }
  }
  myid++;

  if(tnow->next!=NULL && tnow->next->content[0]=='<'){
    char *next_content = tnow->next->content;
    //可以在主要的if外先判斷有沒有user pipe in ，
    //然後把指針往前推一個，就可以繼續判斷有沒有接續的其他pipe
    //use a user pipe and delete it

    int number = atoi(next_content+1); 
    //假如user pipe成立後，user退出，那這個user pipe還算有效嗎？   

    int myid;
    for(myid=0;myid<35;myid++){
      if(users[myid].fd==fd){
        break;
      }	
    }
    myid++;
    u_pipe *using;

    if(users[number-1].exist==0){
      FILE *file=fdopen(fd,"w");
      fprintf(file,"*** Error: user #%d does not exist yet. ***\n",number);
      fflush(file);

      return 1;
    }

    int find_it=0;
    for(u_pipe *i=uroot;i!=NULL;i=i->next){
      if(myid==i->to_id && number==i->from_id){
        using=i;
	find_it=1; 
        break; 
      }
    }
    
    if(find_it==0){
      FILE *file=fdopen(fd,"w");
      fprintf(file,"*** Error: the pipe #%d->#%d does not exist yet. ***\n",number,myid);
      fflush(file);
      return 1;
    }

    last_pfd[1]=using->fd_write;
    last_pfd[0]=using->fd_read;
    
    user_pipe_ocurr=1;

    //broadcast pipe in message
    //順序應該是錯的，應該是要訊息列印出來，再印出broadcast訊息
    char *my_name  = using->to_name;
    char *from_name= using->from_name;
    FILE *file = fdopen(fd,"w");
    for(int i=0;i<35;i++){
      if(users[i].exist==1){
        file=fdopen(users[i].fd,"w");
	fprintf(file,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",my_name,myid,from_name,using->from_id,command);
        fflush(file);
      }
    }

    free_userpipe(using->from_id,using->to_id,&uroot);
    tnow->next=tnow->next->next;
    //在資源回收上有問題，應該要執行一個就把一個node free掉
  }

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
          if(i->number==number && fd==i->fd){
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

          n_pipe *newnode = make_a_pipe(number,pfd[1],pfd[0],fd);
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
          if(i->number==number && i->fd==fd){
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

          n_pipe *newnode = make_a_pipe(number,pfd[1],pfd[0],fd);

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

      
      if(next_content[1]=='\x00'){
        //file redirection

        file_name = tnow->next->next->content;
        pipe_type=2;

      }else{
        //user pipe out
      	
        //check whether the user pipe in after pipe out in exist
	if(tnow->next->next!=NULL  && tnow->next->next->content[0]=='<'){
          //use a user pipe and delete it

          int number = atoi(tnow->next->next->content+1); 
          //假如user pipe成立後，user退出，那這個user pipe還算有效嗎？   

          int myid;
          for(myid=0;myid<35;myid++){
            if(users[myid].fd==fd){
              break;
            }	
          }
          myid++;
          u_pipe *using;

          if(users[number-1].exist==0){
            FILE *file=fdopen(fd,"w");
            fprintf(file,"*** Error: user #%d does not exist yet. ***\n",number);
            fflush(file);

            return 1;
          }

          int find_it=0;
          for(u_pipe *i=uroot;i!=NULL;i=i->next){
            if(myid==i->to_id && number==i->from_id){
              using=i;
	      find_it=1; 
              break; 
            }
          }
    
          if(find_it==0){
            FILE *file=fdopen(fd,"w");
            fprintf(file,"*** Error: the pipe #%d->#%d does not exist yet. ***\n",number,myid);
            fflush(file);
            return 1;
          }

          last_pfd[1]=using->fd_write;
          last_pfd[0]=using->fd_read;
    
          user_pipe_ocurr=1;

          //broadcast pipe in message
          //順序應該是錯的，應該是要訊息列印出來，再印出broadcast訊息
          char *my_name  = using->to_name;
          char *from_name= using->from_name;
          FILE *file = fdopen(fd,"w");
          for(int i=0;i<35;i++){
            if(users[i].exist==1){
              file=fdopen(users[i].fd,"w");
	      fprintf(file,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",my_name,myid,from_name,using->from_id,command);
              fflush(file);
            }
          }
          free_userpipe(using->from_id,using->to_id,&uroot);
        }



        int number = atoi((next_content+1));
	if(users[number-1].exist==0){
          //user not exist         
          FILE *file = fdopen(fd,"w");
          fprintf(file,"*** Error: user #%d does not exist yet. ***\n",number);
          fflush(file);
	  return 1;
 	}else {
	  //try to make a user pipe

	  int toid;
          int myid;

	  toid = number;
          for(myid=0;myid<35;myid++){
            if(users[myid].fd==fd){
              break;
	    }
	  } 
	  myid++;


	  int err_flag=0;
	  /*
	  //user pipe already exist 
	  //drop new request
          for(u_pipe*i=uroot;i!=NULL;i=i->next){
            if(i->from_id==myid && i->to_id==toid){
               err_flag=1;
               FILE *file = fdopen(fd,"w");
               fprintf(file,"*** Error: the pipe #%d->#%d already exists. ***\n",i->from_id,i->to_id); 
               fflush(file);
	       return 1;
               //print error message
	    }
	  }*/

	  //檢查完沒有重複的pipe後，再new新的pipe
   	  if(pipe(pfd)<0){
            printf("pipe error!\n");
	    return 0;
	  }

          if(err_flag==0){ 
            if(uroot==NULL){
              uroot = make_a_userpipe(myid,users[myid-1].name,toid,users[toid-1].name,pfd[1],pfd[0]);
	    }else{
              u_pipe *newnode =make_a_userpipe(myid,users[myid-1].name,toid,users[toid-1].name,pfd[1],pfd[0]);
              newnode->next = uroot;
	      uroot = newnode; 
	    }
	  }
	  pipe_type=5;
	  //do exist
	  
	  //broadcast pipe out message
          char *my_name=users[myid-1].name;
	  char *to_name=users[number-1].name;
	  FILE *file;
          for(int i=0;i<35;i++){
            if(users[i].exist==1){
              file=fdopen(users[i].fd,"w");
	      fprintf(file,"*** %s (#%d) just piped '%s' to %s (#%d) ***\n",my_name,myid,command,to_name,number);
              fflush(file);
	    }
	  }	  

	}
      }
    } 
  }


  if(first_flag==1){
    //check for numbered pipe

    //if has number pipe , set number_pipe = 1
    //set number_pipe's pfd to last_pfd
    //if has no number pipe , set number_pipe = 0
    for(n_pipe *i=proot;i!=NULL;i=i->next){
      if(i->number==0 && fd==i->fd){
	
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

    if(first_flag!=1 /*&& tnow->next!=NULL*/ || number_pipe==1 || user_pipe_ocurr==1){
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
      
      if(first_flag != 1 || number_pipe==1 || user_pipe_ocurr==1){ 
        number_pipe=0;
        close(last_pfd[0]);
        close(last_pfd[1]);
      }
      
      if(tnow->next==NULL || pipe_type==2 /*|| pipe_type==4 || pipe_type==3*/){
	int status;
        pid_t p=waitpid(pid,&status,0);
	//printf("pid : %ld\n",p);
      }

      if(pipe_type==5){
        close(pfd[0]);
        close(pfd[1]);
      }    

      exit(1);

    }else if(!pid){
      //child
      

      //if have no special pipe type , just dup the fd to
      //stdout & stderr
      dup2(fd,1);
      dup2(fd,2);


      //前面有pipe
      //或是有numbered pipe
      if(first_flag!=1 || number_pipe==1 || user_pipe_ocurr==1){
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
      }else if(pipe_type==1 || pipe_type==4 || pipe_type==5){
        //! pipe
	//! numbered pipe
	//>n user pipe

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
      char **envp=users[myid-1].envp;



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
  free_0_pipe(&proot,fd);

  
  return 1;
}


void execute(token *troot,int fd,char *command,int myid){
  token *tnow = troot;
  token *tlast= NULL;
  int last_pfd[2];
  int first_flag=1;
  FILE *file = fdopen(fd,"w");



  //yell and tell;
  if(tnow!=NULL){
    if(strcmp(tnow->content,"yell")==0){
      int index=0;
      for(index=0;index<35;index++){
        if(users[index].fd==fd){
          break;
	}
      }

      //broad cast login message
      for(int i=0;i<35;i++){
        if(users[i].exist==1){
          FILE *file = fdopen(users[i].fd,"w");
	  //*** Apple yelled ***: Who knows how to do project 2? Help me plz!
          fprintf(file,"*** %s yelled ***: %s\n",users[index].name,command+5);
          fflush(file);
        }
      } 
      return;
    }else if(strcmp(tnow->content,"tell")==0){
      int index=5;
      FILE *file=fdopen(fd,"w");
      while(command[index]>='0' && command[index]<='9'){
        index++;
      }
      index++;
    
      /*
      int myid;

      for(myid=0;myid<35;myid++){
        if(users[myid].fd==fd){
          break;
        }
      }
      myid++;*/
       
      int toid = atoi(tnow->parameters->content);

      if(users[toid-1].exist==0){
        //do not exist
        fprintf(file,"*** Error: user #%d does not exist yet. ***\n",toid);
        fflush(file);	
      }else{
        //exist
	FILE *tempfd;
        tempfd = fdopen(users[toid-1].fd,"w");	
	fprintf(tempfd,"*** %s told you ***: %s\n",users[myid-1].name,command+index);
        fflush(tempfd);
      }
      return ;
    }
  }

  

  //look back to see whether '>' exist
  //and look back to see whether have duplicated user pipe
  for(token *i=troot;i!=NULL;i=i->next){
    if(i->content[0]=='>' && i->content[1]=='\x00'){
      int out = open(i->next->content, O_RDWR|O_CREAT, 0600);
      close(out);
    }else if(i->content[0]=='>' && i->content[1]!='\x00'){
      int number = atoi(i->content+1); 
      /*int myid;
      for(myid=0;myid<35;myid++){
        if(users[myid].fd==fd){
          break; 
	}
      }
      myid++;*/


      //user pipe already exist 
      //drop new request
      for(u_pipe*i=uroot;i!=NULL;i=i->next){
        if(i->from_id==myid && i->to_id==number){
          FILE *file = fdopen(fd,"w");
          fprintf(file,"*** Error: the pipe #%d->#%d already exists. ***\n",i->from_id,i->to_id); 
          fflush(file);
          return;
          //print error message
	}
      }

    }
  }

  while (tnow!=NULL) {

    if(strcmp(tnow->content,"exit")==0){
      //(void)close(fd);
      //FD_CLR(fd,&afds);
      int myid;
      for(myid=0;myid<35;myid++){
        if(users[myid].fd==fd){
          break;
	}
      }
      myid++;

      //broad cast login message
      char str[30];
      memset(str,0,30);
      strcat(str," *** User '");
      strcat(str,users[myid-1].name);
      strcat(str,"' left. ***\n");
      fprintf(file,"%s",str);
      fflush(file);
      for(int i=0;i<35;i++){
        if(users[i].exist==1 && i!=(myid-1)){
          FILE *tempfp = fdopen(users[i].fd,"w");
          fprintf(tempfp," *** User '%s' left. ***\n",users[myid-1].name);
          fflush(tempfp);
        }
      } 

      free_user(users,myid);
      free_logout_userpipe(myid,&uroot);
      close(fd);
      FD_CLR(fd,&afds); 
      //exit(0);

    }else if(strcmp(tnow->content,"setenv")==0){
      char *name = tnow->parameters->content;
      char *value= tnow->parameters->next->content;
      cus_setenv(name,value,myid);

    }else if(strcmp(tnow->content,"printenv")==0){

      char *name = tnow->parameters->content;
      print_a_env(fd,name,myid);

    }else if(strcmp(tnow->content,"who")==0){
      FILE *file = fdopen(fd,"w");

      fprintf(file,"<ID>\t<nickname>\t<IP/port>\t<indicate me>\n");
      for(int i=0;i<35;i++){
        if(users[i].exist==1){
          if(fd==users[i].fd){

            fprintf(file,"%d\t%s\t%s/%d\t<-me\n",i+1,users[i].name,"CGILAB",511);
          }else{
            fprintf(file,"%d\t%s\t%s/%d\n",i+1,users[i].name,"CGILAB",511);
	  }
        }
      }  
      fflush(file);

    }else if(strcmp(tnow->content,"name")==0){
      FILE *file = fdopen(fd,"w");


      //check whether the user has already exist
      int name_same=0;
      for(int i=0;i<35;i++){
        if(users[i].exist==1 && strcmp(users[i].name,tnow->parameters->content)==0){
          name_same=1;
	  break;
	}
      } 


      if(name_same==1){
        for(int i=0;i<35;i++){
          if(users[i].fd==fd){
            fprintf(file,"*** User '%s' already exists. ***\n",tnow->parameters->content);
            fflush(file);
	    break;
	  }
	} 
      }else{
        for(int i=0;i<35;i++){
          if(users[i].fd==fd){

  	    //broad cast login message
            for(int j=0;j<35;j++){
              if(users[j].exist==1){
		FILE *broadcast = fdopen(users[j].fd,"w");
                fprintf(broadcast,"*** User from %s/%d is named '%s'. ***\n","CGILAB",511,tnow->parameters->content);
                fflush(broadcast);
              }
            }

            free(users[i].name);
	    users[i].name = (char *)malloc(strlen(tnow->parameters->content)+5);
	    strcpy(users[i].name,tnow->parameters->content);
	    break;
	  }
        } 


      }
      //printf("%s\n",tnow->parameters->content);

    }else if(tnow->content[0]=='|'){

    }else if(tnow->content[0]=='>'){
      tnow->next=NULL;//結束執行
    }else if(tnow->content[0]=='<'){
      tnow->next=NULL;
    }else if(tnow->content[0]=='!'){

    }else {

      //get path
      char *path=(char *)malloc(sizeof(char)*500);
      int get_env_result=get_a_env("PATH",myid,path);

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
      fork_subroutine(first_flag,last_pfd,file_path,tnow,file_exist_flag,fd,command); 
 
      //假如fork的過程中出現錯誤，就跳出迴圈
      //if(result==1) break;  


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

