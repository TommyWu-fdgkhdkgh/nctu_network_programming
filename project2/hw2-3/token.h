#ifndef TOKEN_H
#define TOKEN_H
typedef struct TOKEN {
  int type;
  char *content;
  struct TOKEN *parameters;
  struct TOKEN *next;
}token;

struct TOKEN *tokenlize(char *command);
void free_all_token(struct TOKEN *troot);
void free_a_token(struct TOKEN *freetoken);
void display_tokens(struct TOKEN *troot);
struct TOKEN *make_a_token(char *content);
#endif
