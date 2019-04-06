#include "execute.h"
#include "get_command.h"
#include "env.h"
#include "pipe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
n_pipe *proot=NULL;

int main() {
  size_t len = 20000;
  char command[len];
  char in;
  int index;
  token *troot;

  char *name = "PATH";
  char *value= "bin:.";
  cus_setenv(name,value);

  while (1){
    //get input
    get_command(command);
    //translate input to tokens
    troot=tokenlize(command);

    int n_of_token=0;
    for(token *i=troot;i!=NULL;i=i->next){
      n_of_token++;
    }

    if(n_of_token!=0){

      //execute command
      execute(troot);
     
      //free all token
      free_all_token(troot);
      troot=NULL;

      fresh_all_pipe(proot);

    }/*else {
      printf("zero!\n");
    }*/

    
    //display_tokens(troot);

    //printf("your command is :%s\n",command);
  }
  return 0;
}
