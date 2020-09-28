#include <isa.h>
#include <string.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdlib.h>

word_t isa_reg_str2val(const char *s, bool *success);
bool paddr_reachable(paddr_t addr);
word_t paddr_read(paddr_t addr, int len);


enum {
  TK_NOTYPE = 256, TK_EQ,TK_GT,TK_LT,TK_UEQ,TK_AND,TK_OR,
  TK_HEX_INT32,TK_INT32,TK_NEG,
  TK_REG_VAR,TK_REG_ZERO,TK_REG_ADDR,TK_GET_VALUE,
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {
  // RISCV32 Registers
  {"\\${1,2}0",TK_REG_ZERO},
  {"\\$[a-z]{1}[0-9]{1,2}",TK_REG_VAR},
  {"\\$[a-z]{2}",TK_REG_ADDR},

  {">",TK_GT},
  {"<",TK_LT},
  {"\\!=",TK_UEQ},
  {"\\&\\&",TK_AND},
  {"\\|\\|",TK_OR},

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"-",'-'},
  {"/",'/'},
  {"\\*",'*'},
  {"\\(",'('},
  {"\\)",')'},
  {"0x[0-9a-f]+",TK_HEX_INT32},
  {"[0-9]+",TK_INT32}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i].
         * Add codes to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        int token_type = rules[i].token_type;
        switch (token_type) {
          case TK_NOTYPE:
              break;
          // 如果前一个token不是)或者数，则将该'-'转换为负数符号
          case '-': {
              if (nr_token == 0 || (tokens[nr_token - 1].type != ')'
                                    && tokens[nr_token - 1].type != TK_HEX_INT32
                                    && tokens[nr_token - 1].type != TK_INT32))
                  token_type = TK_NEG;
          }
          default:{
              if (substr_len>=32)
                  printf("Length of token out of limit (32).");
              Token new_token;
              new_token.type = token_type;
              strncpy(new_token.str,substr_start,substr_len);
              new_token.str[substr_len] = '\0';
              tokens[nr_token]=new_token;

              // Handling the * and address
              if ((new_token.type == TK_HEX_INT32 || new_token.type == TK_REG_ADDR)
                    && nr_token > 0 && tokens[nr_token-1].type=='*'){
                  if ((nr_token-1)==0)
                      tokens[nr_token-1].type = TK_GET_VALUE;
                  else if((nr_token-2)>=0){
                      int prev_type = tokens[nr_token-2].type;
                      if (prev_type=='+'||prev_type=='-'||prev_type=='*'
                            ||prev_type=='/'||prev_type==TK_EQ||prev_type==TK_UEQ||prev_type==TK_NEG)
                          tokens[nr_token-1].type = TK_GET_VALUE;
                  }
              }
              nr_token++;
          }
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

/* 计算表达式用 */

bool check_bracket_integrity(int p,int q){
    int bracket_l = 0;
    for (int i = p; i <= q; ++i) {
        if (tokens[i].type=='(')
            bracket_l++;
        else if (tokens[i].type==')')
            bracket_l--;
    }
    if (bracket_l == 0)
        return true;
    else
        return false;
}

bool check_parentheses(int p,int q){
    if (p>=q)
        return false;   // bad expression
    else if (tokens[p].type !='(' ||  tokens[q].type != ')')
        return false;
    else
        // Continue checking expr with stack.
        return check_bracket_integrity(p,q);
}

int is_op_symbol(int p){
    if (p < 0 || p >= nr_token)
        return false;
    int t = tokens[p].type;
    if (t == TK_EQ||t==TK_UEQ)
        return 1;
    else if (t == '+' || t=='-')
        return 2;
    else if (t=='*'||t=='/')
        return 3;
    else
        return 10;
}

int divide_expr(int p ,int q){
    if (check_parentheses(p,q))
        return divide_expr(p+1,q-1);
    // +,-,*,/ all need at least 3 tokens.
    if (q-p <= 1)
        return -1;
    else if(!check_bracket_integrity(p,q))
        return -2;
    // 从右向左分割，保证计算从左向右
    // 考虑运算优先级
    int lowest_priority_pos = -1;
    int lowest_priority = 5;

    for (int i = q; i >= p; --i) {
        int b = is_op_symbol(i);
        if (b < lowest_priority && check_bracket_integrity(p,i-1)){
            lowest_priority = b;
            lowest_priority_pos = i;
        }

        if ((i==p||i==q) && b<5)
            return -2;
        // 匹配最左侧的最低优先级运算符
    }
    if (lowest_priority_pos > p && lowest_priority <5){
        return lowest_priority_pos;
    }

    // Obviously, the expr that cannot be divided into to exprs is likely to be a minimum expr.
    return -3;
}

word_t op_uni(struct token t,bool* success){
    *success = true;
    if (t.type==TK_REG_ZERO) {
        return 0;
    }
    else if (t.type==TK_INT32){
        return strtol(t.str,NULL,10);
    }
    else if(t.type==TK_HEX_INT32){
        return strtol(t.str,NULL,16);
    }
    else if (t.type==TK_REG_VAR || t.type==TK_REG_ADDR){
        return isa_reg_str2val(t.str+1,success);
    }
    else{
        *success=false;
        return 0;
    }
}

word_t op_tri(word_t a,word_t b,struct token* op){
    switch (op->type) {
        case '+':return a+b;
        case '-':return a-b;
        case '*':return a*b;
        case '/':return a/b;
        case TK_EQ:return (word_t)(a==b);
        case TK_UEQ:return (word_t)(a!=b);
        default:return 0;   // 通过is_op_symbol进行分割时已经限制二元运算符的种类
    }
}

word_t eval(int p, int q,bool* success) {
    if (p > q || p < 0 || q < 0) {
        *success=false;
        return 0;
    }
    else if (p == q) {
        return op_uni(tokens[p],success);
    }
    else if (check_parentheses(p, q) == true) {
        return eval(p + 1, q - 1,success);
    }
    else {
        int div = divide_expr(p,q);
        // printf("Divide expr return val:%d.\n",div);
        if (div > 0){
            // 分割表达式成功

            bool success_a=false,success_b=false;
            word_t res_a = eval(p,div-1,&success_a);
            word_t res_b = eval(div+1,q,&success_b);
            if (success_a && success_b){
                *success = true;
                return op_tri(res_a,res_b,&tokens[div]);
            }
        }
        else if (div==-3 || ((div==-1) && (p<q))){
            // 判断是否为一元运算符开头的表达式
            bool success_bi=false;
            word_t res_bi = eval(p+1,q,&success_bi);
            if (tokens[p].type==TK_GET_VALUE && success_bi){
                if (paddr_reachable(res_bi)) {
                    *success = true;
                    return paddr_read(res_bi,4);
                }
            }
            else if (tokens[p].type==TK_NEG && success_bi){
                // todo 实现带有负数的算术表达式的求值 (选做)
            }
        }
        *success=false;
        return 0;
    }
}



word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  word_t result = 0;
  bool success_eval = false;
  /*  Debug log.
  for (int i = 0; i < nr_token; ++i) {
      printf("token type:%d  token:%s\n",tokens[i].type,tokens[i].str);
  }
   */
  // To avoid the func being called every time during loop.
  if (check_bracket_integrity(0,nr_token-1) == true) {
      // printf("Brackets checking success.\n");
      result = eval(0, nr_token - 1, &success_eval);
  }
  if (success_eval){
      *success=true;
      // printf("Result:%d\n",result);
  }
  else{
      *success = false;
      printf("Expr.c: func expr(): Failed to evaluate the expression.\n");
  }
  return result;
}
