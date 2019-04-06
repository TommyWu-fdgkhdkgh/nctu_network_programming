#include "pipe.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void fresh_all_pipe(n_pipe *proot,int fd){
  for(n_pipe *i=proot;i!=NULL;i=i->next){
    if(fd==i->fd){
      i->number=i->number-1;
    } 
  }
}

void free_0_pipe(n_pipe **p2proot,int fd){
  n_pipe *pnow;
  n_pipe *plast;

  plast = NULL;
  for(n_pipe *i=*p2proot;i!=NULL;){

    //pnow = i;
    //printf("he!") 
    if(i->number==0 && fd==i->fd){
      //printf("free it!\n");
      if(plast==NULL){
        *p2proot = i->next;
      }else{
        plast->next = i->next; 
      }
      pnow=i->next;
      free(i);
      i=pnow;
    }else{

      plast = i;
      i=i->next;
    }
  }

}


void free_a_pipe(n_pipe *freepipe,n_pipe **p2proot){
}

n_pipe *make_a_pipe(int number,int fd_write,int fd_read,int fd){

  n_pipe *newnode = (n_pipe *)malloc(sizeof(n_pipe));
  newnode->next    =NULL;
  newnode->number  =number;
  
  newnode->fd_write=fd_write;
  newnode->fd_read =fd_read;
  
  newnode->fd      =fd;

  return newnode;
}


u_pipe *make_a_userpipe(int from_id,char *from_name,int to_id,char *to_name,int fd_write,int fd_read){
  u_pipe *newnode = (u_pipe*)malloc(sizeof(u_pipe));

  newnode->next    = NULL;
  newnode->from_id = from_id;
  newnode->from_name = (char *)malloc(strlen(from_name)+5); 
  strcpy(newnode->from_name,from_name);
  newnode->to_id   = to_id;
  newnode->to_name = (char *)malloc(strlen(to_name)+5);
  strcpy(newnode->to_name , to_name);

  newnode->fd_write = fd_write;
  newnode->fd_read  = fd_read; 

}

void free_userpipe(int from_id,int to_id,u_pipe **p2proot){
  u_pipe *pnow;
  u_pipe *plast;

  plast = NULL;
  for(u_pipe *i=*p2proot;i!=NULL;){
    if(i->from_id==from_id && i->to_id==to_id){
      //printf("free it!\n");
      if(plast==NULL){
        *p2proot = i->next;
      }else{
        plast->next = i->next; 
      }
      
      free(i->to_name);
      free(i->from_name);
      free(i);

      break;
    }else{
      plast=i;
      i=i->next;
    }
  }
}

void free_logout_userpipe(int id,u_pipe **p2proot){

  u_pipe *plast=NULL;
  u_pipe *temp=NULL;
  for(u_pipe *i=*p2proot;i!=NULL;){
    if(i->from_id==id || i->to_id==id){
      close(i->fd_write);
      close(i->fd_read);
   
      if(plast==NULL){
        *p2proot = i->next;
      }else{
        plast->next = i->next; 
      }
      temp=i->next; 

      free(i->to_name);
      free(i->from_name);
      free(i);

      i=temp;
    }else{
      plast=i;
      i=i->next;
    }

  }
}
