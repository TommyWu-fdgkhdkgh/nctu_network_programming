#include "env.h"
#include "execute.h"
#include "get_command.h"
#include "pipe.h"
#include "hi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_command(char *command) {
  char in;
  int index;

  fflush(stdout);
  printf("%% ");
  fflush(stdout); 

  index = 0;
  in = getchar();
  while(in!='\n' && in!=EOF && in!='\x00' && in!='\r') {
    command[index]=in;
    index++;
    in = getchar();
  }
  command[index]='\x00';
}

