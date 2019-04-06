#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(){

  int *a = (int *)malloc(sizeof(int)*900);
  int b[30][30];

  for(int i=0;i<30;i++){
    for(int j=0;j<30;j++){
      *(a+30*i+j)=i*j;
      b[i][j]=i*j;
    }
  }

  printf("%d %d\n",*(a+30*1+5),b[1][5]);


  return 0;
}
