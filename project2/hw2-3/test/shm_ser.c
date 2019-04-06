#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shm.h"
#include "user.h"

#define SEMKEY 123456L 
#define PERMS 0666 //???
#define BIGCOUNT 111


extern int errno;
int semid = -1; /*semaphore id*/
int shmid = -1;


static struct sembuf op_lock[2]={
  2,0,0,                           /*wait for sem#0 to become 0*/
  2,1,SEM_UNDO                     /*then increment sem#0 by1*/
};

static struct sembuf op_unlock[1]={
  2,-1,SEM_UNDO
};                                 /*decrement sem#0 by 1 (sets it to 0)*/

static struct sembuf op_endcreate[2]={
  1,-1,SEM_UNDO,
  2,-1,SEM_UNDO
};

static struct sembuf op_open[1]={
  1,-1,SEM_UNDO
};

static struct sembuf op_close[3]={
  2,0,0, 
  2,1,SEM_UNDO,
  1,1,SEM_UNDO
};

static struct sembuf op_op[1]={
  0,99,SEM_UNDO
};


void err_sys(char *x);
int sem_create(key_t key, int initval);
void sem_rm(int id);
int sem_open(key_t key);
void sem_close(int id);
void sem_op(int id,int value);
void sem_wait(int id);
void sem_signal(int id);
void my_lock(int fd);
void my_unlock(int fd);
void server();

int main(){

  //create a share memory , size的大小是Mesg(可自訂)
  if((shmid=shmget(SHMKEY,sizeof(Mesg),PERMS | IPC_CREAT))<0)
    err_sys("server:can't get shared memory\n");

  //pointer 可以直接指向 share memory
  if((mesgptr=(Mesg *)shmat(shmid ,(char *)0,0))==-1) 
    err_sys("...\n");

  //create a semaphore
  if((clisem = sem_create(SEMKEY1,1))<0)
    err_sys("...\n");

  //create a semaphore
  if((servsem = sem_create(SEMKEY2,0))<0)
    err_sys("...\n");

  server();

  if(shmdt(mesgptr)<0)
    err_sys("server:can't detach shared memory\n");

  sem_close(clisem);
  sem_close(servsem);

  
  return 0;
}

void server(){
  int n,filefd;
  char errmesg[256];

  sem_wait(servsem);
  mesgptr->mesg_data[mesgptr->mesg_len]='\0';

  if((filefd=open(mesgptr->mesg_data,0))<0){
    //Error. Format an error message and send it back to the client.
    printf("can't open\n");

    sem_signal(clisem); /*send to client*/
    sem_wait(servsem);  /*wait for client to process*/

  }else{
    while((n=read(filefd,mesgptr->mesg_data,MAXMESGDATA-1))>0){
      mesgptr->mesg_len=n;
      sem_signal(clisem); /*send to client*/
      sem_wait(servsem);  /*wait for client to process*/
    }
    close(filefd);
    if(n<0) err_sys("server: read error\n");

  }

  mesgptr->mesg_len=0;
  sem_signal(clisem);

}


int sem_create(key_t key , int initval){
  int id,semval;
  union semun semctl_arg; 

  if(key==IPC_PRIVATE)  return -1;
  else if(key==(key_t) -1) return -1;

  int flag;
  do{
    flag=0;
    if((id=semget(key,3,0666|IPC_CREAT))<0) return -1;
    if(semop(id,&op_lock[0],2)<0){
      if(errno==EINVAL){
        flag=1;
      }
    }
  }while(flag);
  if(errno == EINVAL){
    err_sys("can't lock\n");
  }

  if((semval=semctl(id,1,GETVAL,0))<0) err_sys("can't GETVAL\n");
  if(semval==0){
    semctl_arg.val = initval;
    if(semctl(id,0,SETVAL,semctl_arg)<0)
      err_sys("can't SETVAL[0]\n");
    semctl_arg.val=BIGCOUNT; 
    
    if(semctl(id,1,SETVAL,semctl_arg)<0)
      err_sys("can't SETVAL[1]\n");
  }
  if(semop(id,&op_endcreate[0],2)<0)
    err_sys("can't end create\n");

  return id;
}

void sem_rm(int id){
  if(semctl(id,0,IPC_RMID,0)<0)
    err_sys("can't IPC_RMID\n");
}

int sem_open(key_t key){
  int id;
  if(key==IPC_PRIVATE) return -1;
  else if(key==(key_t)-1) return -1;

  if((id=semget(key,3,0))<0) return -1; 

  if(semop(id,&op_open[0],1)<0) err_sys("can't open\n");

  return id;
}
void sem_close(int id){
  int semval;

  if(semop(id,&op_close[0],3)<0) err_sys("can't semop\n");

  if((semval=semctl(id,1,GETVAL,0))<0) err_sys("can't GETVAL\n");

  if(semval>BIGCOUNT) err_sys("sem[1] > BIGCOUNT");
  else if(semval==BIGCOUNT) sem_rm(id);
  else
    if(semop(id,&op_unlock[0],1)<0) err_sys("can't unlock\n");

}

void sem_op(int id,int value){
  if((op_op[0].sem_op=value)==0) err_sys("can't have value == 0");
  if(semop(id,&op_op[0],1)<0) err_sys("sem_op error");
}

void sem_wait(int id){
  sem_op(id,-1);
}

void sem_signal(int id){
  sem_op(id,1);
}

void my_lock(int fd){
  if(semid<0){
    if((semid=sem_create(SEMKEY,1))<0) err_sys("sem_create error");
  }
  sem_wait(semid);
}

void my_unlock(int fd){
  sem_signal(semid);
}

void err_sys(char *x){
  fprintf(stderr,x);
  exit(1);
}

