#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(){

  char str[20]="ddddddd\n";

  char *ss = "gg\n";
  char *gg = "ss\n";

  strcpy(str,ss);
  strcat(str,gg);

  printf("%s\n",str);


  return 0;
}
