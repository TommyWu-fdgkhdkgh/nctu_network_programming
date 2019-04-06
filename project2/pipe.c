#include "pipe.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fresh_all_pipe(n_pipe *proot){
  for(n_pipe *i=proot;i!=NULL;i=i->next){
    i->number=i->number-1; 
  }
}

void free_0_pipe(n_pipe **p2proot){
  n_pipe *pnow;
  n_pipe *plast;

  plast = NULL;
  for(n_pipe *i=*p2proot;i!=NULL;){
    //pnow = i;
    //printf("he!") 
    if(i->number==0){
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

n_pipe *make_a_pipe(int number,int fd_write,int fd_read){

  n_pipe *newnode = (n_pipe *)malloc(sizeof(n_pipe));
  newnode->next    =NULL;
  newnode->number  =number;

  newnode->fd_write=fd_write;
  newnode->fd_read =fd_read;

  return newnode;
}

