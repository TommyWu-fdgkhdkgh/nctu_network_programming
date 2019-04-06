#ifndef USER_H
#define USER_H

typedef struct USER{
  int fd;
  char *name;
  char *ip;
  int port;
  int exist;
  char *envp[100];
}user;

void free_user(struct USER *users,int id);


#endif
