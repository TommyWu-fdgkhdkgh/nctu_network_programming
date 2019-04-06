#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "shm.h"
#include "env.h"
#include "token.h"
#include "pipe.h"
#include "hi.h"
#include "user.h"

extern n_pipe *proot;
extern u_pipe *uroot;
extern char **environ;
extern char *message;
extern user *userptr;
extern int *upipetable;
extern fd_set fds;
extern user *userptr;
extern int myid;
extern int errno;
extern int semid;
extern int shmid;
extern int upipesemid;
extern int upipeshmid;
extern int usercount;
extern int broadsemid;
extern int broadshmid;

int first_str_flag; /* first str use strcpy , and then user the strcat 
                       在執行前再把所有的string列印出來，所以每次execve完一個程式後，再把這個int設為1  */
                    

void my_lock(user *userptr);
void my_unlock(user *userptr);
void my_upipe_lock(int *upipetable);
void my_upipe_unlock(int *upipetable);
void my_message_lock(char *message);
void my_message_unlock(char *message);
void my_broad_lock();
void my_broad_unlock();

int fork_subroutine(int first_flag,int last_pfd[2],char *file_path,token *tnow,int file_exist_flag,int fd,char *command){

  int pfd[2];
  int pipe_type=-1;
  char *file_name;
  int number_pipe=0;
  int user_pipe_ocurr=0;
  int upipefd;
  FILE *file = fdopen(fd,"w");
  char fifoname[10];
  int number;

  if(tnow->next!=NULL && tnow->next->content[0]=='<'){
    char *next_content = tnow->next->content;
    //可以在主要的if外先判斷有沒有user pipe in ，
    //然後把指針往前推一個，就可以繼續判斷有沒有接續的其他pipe
    //use a user pipe and delete it

    int number = atoi(next_content+1); 
    //假如user pipe成立後，user退出，那這個user pipe還算有效嗎？   

    //check whether the user does exist
    if(userptr[number-1].exist==0){
      FILE *file=fdopen(fd,"w");
      fprintf(file,"*** Error: user #%d does not exist yet. ***\n",number);
      fflush(file);
      tnow->next=NULL;
      return 1;
    }
   
    char fifoname[10];
    memset(fifoname,0,10);
    sprintf(fifoname,"./user_pipe/%d_%d",number,myid);
    
    //check whether the pipe exist
    if(*(upipetable+30*(number-1)+(myid-1))==0){
      fprintf(file,"*** Error: the pipe #%d->#%d does not exist yet. ***\n;",number,myid);
      fflush(file);
      tnow->next=NULL;
      return 1;
    }
    
    upipefd = open(fifoname,0);

    //update upipe table
    my_upipe_lock(upipetable);
    *(upipetable+30*(number-1)+(myid-1))=0;
    my_upipe_unlock(upipetable);


    //broadcast user pipe success message
    fprintf(file,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",userptr[myid-1].name,myid,userptr[number-1].name,number,command);
    fflush(file);


    char loginstr[1050];
    memset(loginstr,0,1050);
    sprintf(loginstr,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",userptr[myid-1].name,myid,userptr[number-1].name,number,command);

    if(first_str_flag==1){
      my_message_lock(message);
      strcpy(message,loginstr);
      my_message_unlock(message);
      first_str_flag=0;
    }else{
      my_message_lock(message);
      strcat(message,loginstr);
      my_message_unlock(message);
    }


    //set yell user
    /*my_lock(userptr);
    userptr[30].pid=getpid();
    my_unlock(userptr);
    for(int i=0;i<30;i++){
      if(userptr[i].exist==1 && i!=myid-1){
        kill(userptr[i].pid,SIGUSR1);
      }
    }*/


    last_pfd[1]= upipefd;
    last_pfd[0]= upipefd;
    
    user_pipe_ocurr=1;

    //broadcast pipe in message
    //順序應該是錯的，應該是要訊息列印出來，再印出broadcast訊息

    //free_userpipe(using->from_id,using->to_id,&uroot);

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

          if(userptr[number-1].exist==0){
            fprintf(file,"*** Error: user #%d does not exist yet. ***\n",number);
            fflush(file);
            tnow->next=NULL;
            return 1;
          }

          char fifoname[10];
          memset(fifoname,0,10);
          sprintf(fifoname,"./user_pipe/%d_%d",number,myid);
          //check whether the pipe exist
          if(*(upipetable+30*(number-1)+(myid-1))==0){
            fprintf(file,"*** Error: the pipe #%d->#%d does not exist yet. ***\n;",number,myid);
	    fflush(file);
            tnow->next=NULL; 
            return 1;
	  }
          
	  //update upipe table
	  my_upipe_lock(upipetable);
	  *(upipetable+30*(number-1)+(myid-1))=0;
          my_upipe_unlock(upipetable);

          upipefd = open(fifoname,0);

          //broadcast user pipe success message
          fprintf(file,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",userptr[myid-1].name,myid,userptr[number-1].name,number,command);
          fflush(file);

          char loginstr[1050];
          memset(loginstr,0,1050);
          sprintf(loginstr,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",userptr[myid-1].name,myid,userptr[number-1].name,number,command);


          if(first_str_flag==1){
            my_message_lock(message);
            strcpy(message,loginstr);
            my_message_unlock(message);
            first_str_flag=0;
          }else{
            my_message_lock(message);
            strcat(message,loginstr);
            my_message_unlock(message);
          }

          //set yell user
          /*my_lock(userptr);
          userptr[30].pid=getpid();
          my_unlock(userptr);
          my_message_lock(message);
          strcpy(message,loginstr);
          my_message_unlock(message);

          for(int i=0;i<30;i++){
            if(userptr[i].exist==1 && i!=myid-1){
             kill(userptr[i].pid,SIGUSR1);
            }
          }*/

          last_pfd[1]= upipefd;
          last_pfd[0]= upipefd;
    
          user_pipe_ocurr=1;
         
	  //free user pipe 
          //順序應該是錯的，應該是要訊息列印出來，再印出broadcast訊息
	}	
      	
        number = atoi((next_content+1));
	if(userptr[number-1].exist==0){
          //user not exist         
          fprintf(file,"*** Error: user #%d does not exist yet. ***\n",number);
          fflush(file);
	  tnow->next=NULL;
	  return 1;
 	}else {
	  //try to make a user pipe
	  //user pipe already exist 
	  //drop new request
	  //user pipe does not exist
	  //make new fifo for it

	  //broadcast pipe out message
          //broadcast user pipe message to everyone

  	  fprintf(file,"*** %s (#%d) just piped '%s' to %s (#%d) ***\n",userptr[myid-1].name,myid,command,userptr[number-1].name,number);
          fflush(file);

          char loginstr[1050];
          memset(loginstr,0,1050);
          sprintf(loginstr,"*** %s (#%d) just piped '%s' to %s (#%d) ***\n",userptr[myid-1].name,myid,command,userptr[number-1].name,number);

          if(first_str_flag==1){
            my_message_lock(message);
            strcpy(message,loginstr);
            my_message_unlock(message);
            first_str_flag=0;
          }else{
            my_message_lock(message);
            strcat(message,loginstr);
            my_message_unlock(message);
          }
          //set yell user
          /*my_lock(userptr);
          userptr[30].pid=getpid();
          my_unlock(userptr);
          my_message_lock(message);
          strcpy(message,loginstr);
          my_message_unlock(message);

          for(int i=0;i<30;i++){
            if(userptr[i].exist==1 && i!=myid-1){
              kill(userptr[i].pid,SIGUSR1);
            }
          }*/

	  pipe_type=5;

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


  //broadcast the message!
  if(first_str_flag==0){
    for(int i=0;i<30;i++){
      if(userptr[i].exist==1 && i!=myid-1){
        kill(userptr[i].pid,SIGUSR1);
      }
    }
  }
  first_str_flag=1;
  

  pid_t pid;
  pid_t pid2;

  //printf("start to fork!\n");

  if(pid2=fork()){

    if(first_flag!=1 /*&& tnow->next!=NULL*/ || number_pipe==1){
      number_pipe=0;
      close(last_pfd[0]);
      close(last_pfd[1]);
    }

    if(user_pipe_ocurr==1 || pipe_type==5){
      close(upipefd);
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

      if(user_pipe_ocurr==1 || pipe_type==5){
	close(upipefd);
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
        dup2(last_pfd[0],0);
        close(last_pfd[0]);
        close(last_pfd[1]);
      }

      if(user_pipe_ocurr==1){
        dup2(upipefd,0);
	close(upipefd);
	unlink(fifoname);
      }

      //後面有pipe
      if(pipe_type==0 || pipe_type==3){
	//normal pipe
	//normal numbered pipe
	
        close(pfd[0]);
        dup2(pfd[1],1);
        close(pfd[1]); 
      }else if(pipe_type==1 || pipe_type==4){
        //! pipe
	//! numbered pipe
	//>n user pipe

        close(pfd[0]);
	dup2(pfd[1],1);//stdout
	dup2(pfd[1],2);//stderr
        close(pfd[1]);
      }else if(pipe_type==5){

	sprintf(fifoname,"./user_pipe/%d_%d",myid,number);   
        //可以在child process(有execve的那個再開fifo，這樣就不會卡死在這裡)i
	//我猜ＸＤＤ

        //check whether the user pipe has exist
	/*int fifo_exist=0;
	if(access(fifoname,F_OK)!=-1){
          //file exist
	  fifo_exist=1;
        }
        if(fifo_exist==1){
          fprintf(file,"*** Error: the pipe #%d->#%d already exists. ***\n",myid,number);
	  fflush(file);
	  tnow->next=NULL;
          return 1;
	}*/
	if(*(upipetable+30*(myid-1)+(number-1))==1){
          fprintf(file,"*** Error: the pipe #%d->#%d already exists. ***\n",myid,number);
	  fflush(file);
	  tnow->next=NULL;
          return 1;
	}

        //update upipe table
	my_upipe_lock(upipetable);
	*(upipetable+30*(myid-1)+(number-1))=1;
	my_upipe_unlock(upipetable);

	

        if ( (mknod(fifoname, S_IFIFO | ALLPERMS, 0) < 0) && (errno != EEXIST))
          fprintf(stderr,"can't create fifo: %s", fifoname); 

	upipefd = open(fifoname,1);
        if(upipefd<0){
          fprintf(file,"open error!\n");
	  fflush(file);
	}

        dup2(upipefd,1);
	dup2(upipefd,2);
  	close(upipefd);



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
  free_0_pipe(&proot,fd);

  
  return 1;
}


int execute(token *troot,int fd,char *command){
  token *tnow = troot;
  token *tlast= NULL;
  int last_pfd[2];
  int first_flag=1;
  FILE *sockfp = fdopen(fd,"w");
  char fifoname[10];
  memset(fifoname,0,10);

  //initialize first_str_flag
  first_str_flag=1;

  if(tnow!=NULL &&  strcmp(tnow->content,"yell")==0){
    //broadcast login message
    fprintf(sockfp,"*** %s yelled ***: %s\n",userptr[myid-1].name,command+5);
    fflush(sockfp);

    char loginstr[1050];
    memset(loginstr,0,1050);
    sprintf(loginstr,"*** %s yelled ***: %s\n",userptr[myid-1].name,command+5);

    //set yell user
    my_lock(userptr);
    userptr[30].pid=getpid();
    my_unlock(userptr);

    my_message_lock(message);
    strcpy(message,loginstr);
    my_message_unlock(message);

    for(int i=0;i<30;i++){
      if(userptr[i].exist==1 && i!=myid-1){
        kill(userptr[i].pid,SIGUSR1);
      }
    }

    return 1;
  }else if(tnow!=NULL && strcmp(tnow->content,"tell")==0){

    int index=5;
    FILE *file=fdopen(fd,"w");
    while(command[index]>='0' && command[index]<='9'){
      index++;
    }
    index++;

    int toid = atoi(tnow->parameters->content);

    //check whether the user exist
    if(userptr[toid-1].exist==0){
      fprintf(sockfp,"*** Error: user #%d does not exist yet. ***\n",toid);
      fflush(sockfp);
    }else{
      //tell something to the user
      
      char loginstr[1050];
      memset(loginstr,0,1050);
      sprintf(loginstr,"*** %s told you ***: %s\n",userptr[myid-1].name,command+index);

      //set yell user
      my_lock(userptr);
      userptr[30].pid=getpid();
      my_unlock(userptr);

      my_message_lock(message);
      strcpy(message,loginstr);
      my_message_unlock(message);
   
      kill(userptr[toid-1].pid,SIGUSR1);
    }
    return 1;
  }

  //look back to see whether '>' exist
  //and see wheter the user pipe out exist
  //to avoid duplicated pipe 
  for(token *i=troot;i!=NULL;i=i->next){
    if(i->content[0]=='>' && i->content[1]=='\x00'){
      int out = open(i->next->content, O_RDWR|O_CREAT, 0600);
      close(out);
    }else if(i->content[0]=='>' && i->content[1]!='\x00'){
      int number=atoi(i->content+1);

      //check whether the user exist
      if(userptr[number-1].exist==0){
        fprintf(sockfp,"*** Error: user #%d does not exist yet. ***\n",number);
	fflush(sockfp);
	return 1;
      }

      //check whether the pipe exist
      if(*(upipetable+30*(myid-1)+(number-1))==1){
        fprintf(sockfp,"*** Error: the pipe #%d->#%d already exists. ***\n",myid,number);
        fflush(sockfp);
	return 1;
      }

    }else if(i->content[0]=='<' && i->content[1]!='\x00'){
      int number=atoi(i->content+1);
      sprintf(fifoname,"./user_pipe/%d_%d",number,myid);
   
      //check whether the user exist
      if(userptr[number-1].exist==0){
        fprintf(sockfp,"*** Error: user #%d does not exist yet. ***\n",number);
	fflush(sockfp);
	return 1;
      }
      
      //check whether the pipe exist
      if(*(upipetable+30*(number-1)+(myid-1))==0){
      	fprintf(sockfp,"*** Error: the pipe #%d->#%d does not exist yet. ***\n",number,myid);
        fflush(sockfp);
	return 1;
      }

    }
  }


  while (tnow!=NULL) {
    if(strcmp(tnow->content,"exit")==0){
      //broadcast leave message
      fprintf(sockfp,"*** User '%s' left. ***\n",userptr[myid-1].name);
      fflush(sockfp);
      char loginstr[1050];
      memset(loginstr,0,1050);
      sprintf(loginstr,"*** User '%s' left. ***\n",userptr[myid-1].name);

      //set yell user
      my_lock(userptr);
      userptr[30].pid=getpid();
      my_unlock(userptr);

      my_message_lock(message);
      strcpy(message,loginstr);
      my_message_unlock(message);

      for(int i=0;i<30;i++){
        if(userptr[i].exist==1 && i!=myid-1){
          kill(userptr[i].pid,SIGUSR1);
        }
      }
      
      my_upipe_lock(upipetable);
      for(int i=0;i<30;i++){
        *(upipetable+30*(myid-1)+i)=0;
        *(upipetable+30*i+(myid-1))=0;
      } 
      my_upipe_unlock(upipetable);
      

      my_lock(userptr);
      free_user(userptr,myid);
      my_unlock(userptr);
      shutdown(fd,2);
      FD_CLR(fd,&fds); 
      exit(0);

    }else if(strcmp(tnow->content,"setenv")==0){
      char *name = tnow->parameters->content;
      char *value= tnow->parameters->next->content;
      cus_setenv(name,value);

    }else if(strcmp(tnow->content,"printenv")==0){
      
      char *name = tnow->parameters->content;
      print_a_env(fd,name);
      
    }else if(strcmp(tnow->content,"who")==0){

      fprintf(sockfp,"<ID>\t<nickname>\t<IP/port>\t<indicate me>\n");
      for(int i=0;i<30;i++){
        if(userptr[i].exist==1){
          if(i==myid-1){
            fprintf(sockfp,"%d\t%s\t%s/%d\t<-me\n",i+1,userptr[i].name,"CGILAB",511);
          }else{
            fprintf(sockfp,"%d\t%s\t%s/%d\n",i+1,userptr[i].name,"CGILAB",511);
	  }
        }
      }  
      fflush(sockfp);

    }else if(strcmp(tnow->content,"name")==0){

      //check whether the name has exist
      for(int i=0;i<30;i++){
        if(userptr[i].exist==1 && strcmp(userptr[i].name,tnow->parameters->content)==0){
          fprintf(sockfp,"*** User '%s' already exists. ***\n",tnow->parameters->content);
	  fflush(sockfp);
	  return 1;
	}
      } 

      //set yell user
      my_lock(userptr);
      userptr[30].pid=getpid();
      my_unlock(userptr);


      //broad cast message
      char loginstr[1050];
      memset(loginstr,0,1050);
      sprintf(loginstr,"*** User from %s/%d is named '%s'. ***\n","CGILAB",511,tnow->parameters->content);
      my_message_lock(message);
      strcpy(message,loginstr);
      my_message_unlock(message);

      //rename
      for(int i=0;i<30;i++){
        if(userptr[i].exist==1){

	  if(i==myid-1){
            //free(userptr[myid-1].name);
            //write to shared memory , so we lock it
	    my_lock(userptr);
	    //userptr[myid-1].name = (char *)malloc(strlen(tnow->parameters->content)+5);
	    memset(userptr[myid-1].name,0,30);
	    strcpy(userptr[myid-1].name,tnow->parameters->content);
	    my_unlock(userptr);
           
	    //broad cast message 
            fprintf(sockfp,"*** User from %s/%d is named '%s'. ***\n","CGILAB",511,tnow->parameters->content);
            fflush(sockfp);


          }else{
            //broad cast message
            kill(userptr[i].pid,SIGUSR1);
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

      //check whether the file exist
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


      //if fork or pipe error , just pipe or fork again 
      fork_subroutine(first_flag,last_pfd,file_path,tnow,file_exist_flag,fd,command); 
  
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

