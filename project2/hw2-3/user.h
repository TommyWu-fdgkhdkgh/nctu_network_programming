#ifndef USER_H
#define USER_H
#include <sys/types.h>

typedef struct USER{
  pid_t pid;
  char name[30];
  char ip[30];
  int port;
  int exist;
  int fd;
}user;

void free_user(struct USER *users,int id);


#endif
