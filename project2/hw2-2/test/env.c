#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char **environ;


int main(int argc,char *argv[],char *envp[]){

  setenv("SSH_CLIENT","gg",1);

  environ[1]="gggg=gg";


  for(int i=0;environ[i]!=NULL;i++){
    printf("%s\n",environ[i]);
  }

  printf("%s\n",environ[0]);

  char input[10]="abc,d";
  char *p;
  char *q;
  p = strtok(input,",");
  q = strtok(NULL, ",");
  printf("%s\n",p);
  printf("%s\n",q);



  return 0;
}


