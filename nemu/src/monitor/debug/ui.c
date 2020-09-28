#include <isa.h>
#include "expr.h"
#include "watchpoint.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>


void cpu_exec(uint64_t);
void isa_reg_display();
int is_batch_mode();
word_t paddr_read(paddr_t addr, int len);
word_t expr(char *e, bool *success);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

/* Add other commands here.  */

static int cmd_si(char* args){
    char *arg = strtok(NULL, " ");
    // 单步执行
    int N = 0;
    if(arg==NULL)
        N = 1;
    else{
        char *ptr;
        N = strtol(arg,&ptr,10);
    }
    if(N < 1)
        printf("Argument Error:si [N], N should be an integer which is greater than 0.\n");
    else
        cpu_exec(N);
    return 0;
}


static int cmd_p(char* args){
    // 表达式求值命令
    if (args==NULL)
        printf("Argument Error: No expression was given.\n");
    else{
        bool success = false;
        word_t result = expr(args,&success);
        if (success)
            printf("%u\n",result);
        else
            printf("Failed to calculate the expression.\n");
    }
    return 0;
}

static int cmd_info(char* args){
    // Next token.
    if (args == NULL){
        printf("Argument Error: No argument was given.\n");
        return 0;
    }

    char *arg = strtok(NULL, " ");
    if (strcmp(arg,"r")==0){
        // Print registers info.
        isa_reg_display();
    }
    else if (strcmp(arg,"w")==0){
        // Print watchpoints info.
        // todo
    }
    else{
        printf("Argument Error: Please specify the subcommand (r/w).");
    }
    return 0;
}

static int cmd_x(char *args){
    char *arg1 = strsep(&args," ");
    char *arg2 = args;
    if (arg1==NULL || arg2==NULL){
        printf("Argument Error: No enough argument was given.\n");
    }
    int N = strtol(arg1,NULL,10);
    if (N < 1){
        printf("Argument Error: Num of the bytes of the memory to read (N) should be greater than 0.\n");
    }
    else{
        unsigned word_width = 4;
        bool success = false;
        word_t addr = expr(arg2,&success);
        if (success) {
            printf("Base_Address: 0x%08x\nNUM_WORDS: %d \tWORD_LEN: %d bytes\n",addr,N,word_width);
            printf("Offset \tAddress \tWord_Hex \tWord_Dec\n");
            for (int i = 0; i < N; ++i) {
                word_t w = paddr_read(addr, word_width);
                printf("%d\t0x%08x\t0x%08x\t%11u\n",i*word_width,addr,w,w);
                addr += word_width;
            }
        }
        else
            printf("Failed to calculate the expression.\n");
    }
    return 0;
}

/* Commands defined completed. */

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si","Execute N instructions of the program.",cmd_si},
  {"info","Display informations of registers(r) or watchpoints(w).",cmd_info},
  {"p","Calculate the result of given expression.",cmd_p},
  {"x","Get several words at the given memory address.",cmd_x},
  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop() {
  if (is_batch_mode()) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
