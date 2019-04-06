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
  /* #bytes in mesg_data, can be 0 or > 0 */ /* message type, must be > 0 */
} Mesg;

Mesg *mesgptr; 
