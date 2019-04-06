#include "token.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


token *make_a_token(char *content) {
  token *new_token     = (token *)malloc(sizeof(token)+10);
  new_token->type      = 0;
  new_token->content   = (char *)malloc(strlen(content)+10);
  //malloc後面要多加一點，不然好像會被蓋掉
  strcpy(new_token->content,content);
  new_token->parameters= NULL;
  new_token->next      = NULL;

  return new_token;
}

void free_all_token(token *troot){
  token *tnow;
  token *tlast;

  for(token *i=troot;i!=NULL;){ 

    if(i->parameters!=NULL){
      token *ptnow;
      for(token *j=i->parameters;j!=NULL;){
        ptnow=j->next;
        free(j);
	j=ptnow;
      }
    }

    tnow=i->next;
    free(i);
    i=tnow;
  }
}


void free_a_token(token *freetoken){
  //printf("free %s\n",freetoken->content);
  token *ppara = freetoken->parameters;
  token *ppast = NULL;
  while(ppara!=NULL){
    ppast = ppara;
    ppara = ppara->next;
    free_a_token(ppast);
  }

  free(freetoken->content);
  free(freetoken);
}

void display_tokens(token *troot) {
  for(token *i=troot;i!=NULL;i=i->next){
    printf("display tokens %s\n",i->content);
  }
}

token *tokenlize(char *command) {
  //printf("all token from%s\n",command);
  int len = strlen(command);
  char temp[300];
  int state = 0;  //0:input to temp 
                  //1:space after token
  int tindex=0;

  token *troot=NULL;
  token *tnow=NULL;

  //string to tokens
  for(int i=0;i<=len;i++) {
    if(command[i]==' ' || command[i]=='\x00' || command[i]=='\x0a') {
      if(state==1) {
        temp[tindex]='\x00';
        if(troot==NULL) {
          troot=make_a_token(temp);
	  tnow = troot;
	} else {
	  tnow->next=make_a_token(temp);
	  tnow=tnow->next;
	}

        state=0;  
      }
      tindex=0;
    } else {
      if(state==0){
        state=1;
      }
      temp[tindex++]=command[i];
    }
  }
  //translate some tokens to parameters
  state=0;
  token *plast = NULL;
  token *ppara = NULL;
  for (token *pnow=troot;pnow!=NULL;) {
    if(pnow->content[0]!='|' && pnow->content[0]!='>' && pnow->content[0]!='!' && pnow->content[0]!='<'){
      if(state==0){
        state = 1;
      }else if(state==1){
	state = 2;
      }
    } else{
      state = 0;
    }

    if(state==0 || state==1){
      plast=pnow;
      pnow =pnow->next;
    }else if(state==2){
      if(plast->parameters==NULL){
        plast->parameters     = pnow;
	plast->next           = pnow->next;
	pnow                  = pnow->next;
	ppara                 = plast->parameters;
	ppara->next           = NULL;
      }else{
	plast->next           = pnow->next;
        ppara->next           = pnow;
        pnow                  = pnow->next;
        ppara                 = ppara->next;
        ppara->next           = NULL;	
      }
    }
  }

  return troot;

}



