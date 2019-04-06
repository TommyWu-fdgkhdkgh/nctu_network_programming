#include <stdio.h>
#include <unistd.h>
int main(void) {
  printf("Main program started\n");
  char* argv[] = { "ls","-al","./", NULL };
  char* envp[] = { "0", NULL };
  execve("./ls", argv, envp);
  perror("Could not execve");
  return 1;
}
