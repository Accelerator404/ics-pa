#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <common.h>

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char arg[2048];
  word_t val;

} WP;

int set_watchpoint(char* args,bool* success);
bool free_watchpoint(int NO);
void print_all_watchpoint();

#endif
