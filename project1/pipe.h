#ifndef PIPE_H
#define PIPE_H

//說不定還是需要實作priority queue
//linked list怕會TLE
typedef struct numberPipe{
  int number;//之後第幾行會讀到
  int fd_write;//pfd[1]
  int fd_read; //pfd[0]

  //char *stderr_buffer;
  struct numberPipe *next;
  struct numberPipe *last;
}n_pipe;

//make a pipe node
struct numberPipe *make_a_pipe(int number,int fd_write,int fd_read);

//free a pipe node
void free_a_pipe(struct numberPipe *freepipe,struct numberPipe **p2proot);

//free all pipe that number==0
void free_0_pipe(n_pipe **p2proot);

//because we only need output whole string for next executable file
char *get_piped_stdout(struct numberPipe **p2proot,char *stdout_buffer);

//all pipes num -1
void fresh_all_pipe(n_pipe *proot);

#endif
