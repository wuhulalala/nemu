/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/host.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <sys/types.h>
#include <threads.h>

extern uint32_t tr;
uint8_t* guest_to_host(paddr_t paddr);
enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_LPAREN, TK_RPAREN,
  TK_DEC, TK_REG,
  TK_NEQ, TK_AND,
  TK_DEREF, TK_HEX
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\$[a-zA-Z_][a-zA-Z0-9_]*", TK_REG},        // right parenthesis
  {"0[xX][0-9a-fA-F]+", TK_HEX},        // right parenthesis
  {"[+-]?[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?", TK_DEC},        // right parenthesis
  {"\\+", '+'},         // plus
  {"\\*", '*'},        // mul
  {"\\/", '/'},        // divide
  {"-", '-'},        // minus
  {"\\(", TK_LPAREN},        // left parenthesis
  {"\\)", TK_RPAREN},        // right parenthesis
  {"==", TK_EQ},        // equal
  {"==", TK_NEQ},        // right parenthesis
  {"!=", TK_NEQ},        // right parenthesis
  {"&&", TK_AND},        // right parenthesis
};

#define NR_REGEX ARRLEN(rules)

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

static Token tokens[65536] __attribute__((used)) = {};
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
        char *substr_start __attribute__((unused)) = e + position;
        int substr_len = pmatch.rm_eo;
#ifdef DEBUG
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
#endif

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case '+':
          case '-':
          case '*':
          case '/':
          case TK_LPAREN:
          case TK_RPAREN:
          case TK_EQ:
          case TK_NEQ:
          case TK_AND:
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
          case TK_DEC:
          case TK_HEX:
          case TK_REG:
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, e + position, substr_len);
            *(tokens[nr_token].str + substr_len) = 0;
            if (strncmp(tokens[nr_token].str, e + position, substr_len) != 0) {
              Log("fail to copy the string");
              return false;
            }
            nr_token++;
            break;
          case TK_NOTYPE:
            break;
          default: TODO();
        }

        position += substr_len;

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

bool check_parentheses(int p, int q) {
  int stk = 0;
  int init = p;
  if (p > q) {
#ifdef DEBUG
    Log("p should less or equal to q");
#endif
    return false;
  } 

  if (p < 0 || q >= nr_token) {
#ifdef DEBUG
    Log("index out of bound");
#endif
    return false;
  }

  if (tokens[p].type != TK_LPAREN) {
#ifdef DEBUG
    Log("the first token is not (");
#endif
    return false;
  }
  while (p <= q) {
    if (p > init && p < q && stk < 1) {
#ifdef DEBUG
      Log("false, the whole expression should be surrounded by a matched parentheses");
#endif
      return false;
    }
    if (tokens[p].type == TK_LPAREN) {
      stk++;
    } else if (tokens[p].type == TK_RPAREN) {
      if (stk <= 0) {
        Log("Bad expression, There is no matching left parenthesis");
        return false;
      }
      stk--;
    }
    p++;
  }

  if (stk != 0) {
    Log("Bad expression There is no matching right parenthesis");
    return false;
  }
#ifdef DEBUG
  Log("the subexpression from %d to %d is surrounded by a matched parentheses", init, q);
#endif

  return true;
}



int right_end(int p, int q) {
  int stk = 1;
  p++;
  while (stk != 0) {
    if (tokens[p].type == TK_RPAREN) {
      stk--;
    } else if (tokens[p].type == TK_LPAREN) {
      stk++;
    }
    p++;
  }
  if (p > q + 1) {
    panic("right end out of bound");
  }
  return p;
}

int check_priority(int a, int b) {
  if (a == '+' || a == '-') {
    if (b == '+' || b == '-') {
      return 0;
    } else return -1;
  } else {
    if (b == '+' || b == '-') {
      return 1;
    } else return 0;
  }
}

uint64_t eval(int p, int q) {
  if (p > q) {
    /* Bad expression */
    Log("Bad expression");
    return 0;
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    
    uint64_t val;

    switch(tokens[p].type) {
      case TK_DEC:
        sscanf(tokens[p].str, "%lu", &val) ;
        return val;
      case TK_HEX:
        sscanf(tokens[p].str, "%lx", &val) ;
        return val;
      case TK_REG:
        bool success = false;
        val = isa_reg_str2val(tokens[p].str, &success);
        if (success) return val;
        return 0;
      default:
        panic("unkown type");


    }
  } 
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else if (tokens[p].type == TK_DEREF) {
    // wait to test
    paddr_t g_addr = eval(p + 1, q);
    Assert(g_addr >= CONFIG_MBASE && g_addr <= CONFIG_MBASE + CONFIG_MSIZE, "guest memory under Memory Base");
    uint8_t *h_addr = guest_to_host(g_addr);
    return host_read(h_addr, 8);
    

  }
  else {
    /* We should do more things here. */
    int i = p;
    int type = 0;
    int pos = -1;
    while (i <= q) {
      switch (tokens[i].type) {
        case TK_DEC:
        case TK_HEX:
        case TK_REG:
          i++;
          break;

        case TK_LPAREN:
          i = right_end(i, q);
          break;
        case '+':
        case '-':
          if (pos == -1) {
            type = tokens[i].type;
            pos = i;
          } else {
            int cmp = check_priority(tokens[i].type, type);
            if (cmp == 0) {
              type = tokens[i].type;
              pos = i;
            } else if (cmp == 1) {
              panic("There is no operation with lower priority than addition and subtraction.");
              return 0;
            } else {
              type = tokens[i].type;
              pos = i;
            }
          }
          i++;
          break;
        case '*':
        case '/':
          if (pos == -1) {
            type = tokens[i].type;
            pos = i;
          } else {
            int cmp = check_priority(tokens[i].type, type);
            if (cmp == 0) {
              type = tokens[i].type;
              pos = i;
            } 
            if (cmp == -1) {
              panic("There is no operation with higher priority than mul and div.");
            }
          }
          i++;
          break;
        case TK_EQ:
        case TK_NEQ:
        case TK_AND:
          type = tokens[i].type;
          pos = i;
          i = q + 1;
          break;
      } 
    }

    if (pos < p || pos > q) {
      panic("op should between p and q");
    }
    uint64_t left = eval(p, pos - 1);
    uint64_t right = eval(pos + 1, q);

    switch (type) {
      case '+': return left + right;
      case '-': return left - right;
      case '*': return left * right;
      case '/': 
        if (right == 0) return 0;
        return (uint64_t)((int)left / (int)right);
      case TK_EQ:
        if (left == right) return 1;
        return 0;
      case TK_NEQ:
        if (left != right) return 1;
        return 0;
      case TK_AND:
        if (left == 0) return 0;
        return right != 0 ? 1 : 0;
      default: 
        panic("unknown operation");
    }

  }
  return 0;
}




word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != TK_DEC && tokens[i - 1].type != TK_HEX && tokens[i - 1].type != TK_REG) ) ) {
      tokens[i].type = TK_DEREF;
    }
  }

  uint64_t result __attribute__((unused)) = (uint64_t)eval(0, nr_token - 1);
  *success = true;
  // printf("$%d = %lu\n", tr, result);
  return result;
}
