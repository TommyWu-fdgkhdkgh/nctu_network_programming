#ifndef SHM_H
#define SHM_H

#define SHMKEY ((key_t) 7890)
#define SHMKEY1 ((key_t) 7891)
#define SHMKEY2 ((key_t) 7892)
#define PERMS 0666 
int shmid, clisem, servsem;
#define MAXMESGDATA (4096-16)
#define MESGHDRSIZE (sizeof(Mesg) - MAXMESGDATA)
typedef struct {
  int mesg_len;
  long mesg_type;
  char mesg_data[MAXMESGDATA];
  
} Mesg;

Mesg *mesgptr; 

typedef struct {
  int from_id;
  int to_id;
  int id; 
}fifo_info;

fifo_info *infoptr;

#endif
