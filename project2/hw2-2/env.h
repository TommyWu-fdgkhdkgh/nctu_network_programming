#ifndef ENV_H
#define ENV_H

typedef struct ENV{
  char *name;
  char *value;
  struct ENV *next;
}env;

//print a env
//print the value depend on the name
void print_a_env(int fd,char *name,int myid);

//if the name doesn't exist , make a env
//if the name exist , set a env
struct ENV  *make_or_set_a_env(char *value);

void cus_setenv(char *name,char *value,int myid);

int get_a_env(char *name,int myid,char *returnstr);
#endif
