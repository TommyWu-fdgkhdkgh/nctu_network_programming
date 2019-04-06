#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>


//假如沒有handle函式的話，那預設會讓process掛掉
//假如有handler的話，那process會繼續活著
//假如有handler但沒做任何事的話，那process還是會繼續活著
void handle(int singo){
  printf("hihi\n");
}

int main(){
  signal(SIGUSR1,handle);

  kill(0,SIGUSR1);

  while(1);

  return 0;
}
