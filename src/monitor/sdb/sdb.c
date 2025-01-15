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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include "sdb.h"

static int is_batch_mode = false;
#ifdef CONFIG_FTRACE
extern char *init;
extern uint64_t nc;
#endif
uint8_t* guest_to_host(paddr_t paddr);
paddr_t host_to_guest(uint8_t *haddr);
void init_regex();
void init_wp_pool();
void init_decode();
WP* new_wp(char *str);
void free_wp(WP *wp);
WP* number2addr(int n);
void watchpoint_display();
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

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);
static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  {"si", "Let the program execute N instructions in a single step and then pause. When N is not given, the default is 1", cmd_si },
  {"info", "info r: Print register status info w: Print monitoring point information", cmd_info},
  {"x", "Find the value of the expression EXPR, use the result as the starting memory address, \
  and output N consecutive 4-byte values ​​in hexadecimal format", cmd_x},
  {"p", "Find the value of the expression EXPR", cmd_p},
  {"w", "Set up monitoring points", cmd_w},
  {"d", "Deleting a Watchpoint", cmd_d},
  /* TODO: Add more commands */

};
WP* new_wp(char *str);

#define NR_CMD ARRLEN(cmd_table)

uint32_t tr = 0;

static int cmd_w(char *args) {
  char *arg = strtok(args, "\"");
  if (arg == NULL) {
    Log("expected: p expr");
    return 0;
  } 

  WP* wp = new_wp(arg);
  Assert(wp, "There is no empty watchpoint");


  return 0;

}

static int cmd_d(char *args) {
  char *arg = strtok(args, " ");
  if (arg == NULL) {
    printf("expect: d N\n");
    return 0;
  }
  int n = atoi(arg);
  if (n < 0 || n >= NR_WP) {
    printf("wrong watchpoint number\n");
    return 0;
  }

  free_wp(number2addr(n));
  printf("delete watchpoint %d\n", n);


  
  return 0;
}

static int cmd_p(char *args) {
  char *arg = strtok(args, "\"");
  bool success;
  if (arg == NULL) {
    Log("expected: p expr");
    return 0;
  } 

  uint64_t result = expr(arg, &success);
  if (!success) {
    printf("something wrong in make_token\n");
  }
  printf("$%d = %lu\n", tr, result);
  tr++;
  
  return 0;
}

static int cmd_x(char *args) {
  char *arg = strtok(args, " ");
  paddr_t addr;
  if (arg == NULL) {
    Log("expected: N > 0");
    return 0;
  }
  int N = atoi(arg);
  if (N <= 0) {
    Log("expected: N > 0");
    return 0;
  }
  arg = strtok(NULL, " ");
  if (arg == NULL) {
    Log("expected: x N addr");
    return 0;
  }
  if (sscanf(arg,"%x", &addr) != 1) {
    Log("invalid address format");
    return 0;
  }

  if ((unsigned long)addr < CONFIG_MBASE) {
    Log("The memory address cannot be accessed");
    return 0;
  }

  uint8_t *host_addr = guest_to_host(addr);
  if (host_addr == NULL) {
      Log("Error: Failed to convert guest address to host address.");
      return 0;
  }
  uint32_t *p = (uint32_t *)host_addr;
  uint32_t *end = (uint32_t *)(host_addr + N * sizeof(uint32_t));
  while (p < end) {
    printf("0x%08lx: \t", (unsigned long)host_to_guest((uint8_t *)p));

    for (int j = 0; j < 4 && p < end; j++, p++) {
        printf("0x%08x\t", *p);
    }

    printf("\n");
  }
  return 0;
}

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

static int cmd_si(char *args) {
  char *arg = strtok(args, " ");
  if (arg == NULL) {
    cpu_exec(1);
  } else {
    int n = atoi(arg);
    cpu_exec(n);
  }
  return 0;
}

static int cmd_info(char *args) {
  char *arg = strtok(args, " ");
  if (arg == NULL) {
    Log("Need more arguments. expected: info w / r");
    return -1;
  } else {
    char c = arg[0];
    switch(c) {
      case 'r':
        isa_reg_display();
        break;
      case 'w':
        watchpoint_display();
        break;
      default:
        Log("wrong argument");
    }
  }
  return 0;  
}










void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
#ifdef CONFIG_FTRACE
    *(init + nc)= 0;
    FILE *file = fopen("/tmp/re.txt", "w");
    if (file == NULL) {
        perror("Error redirecting stdout");
        return;
    }

    printf("This will be written to the file.\n");
    fprintf(file, "%s", init);
    fclose(file);
    free(init);
#endif
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

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { 
          return; 
        }
        break;
      }
    }
#ifdef CONFIG_FTRACE
    if (strcmp(cmd, "c") == 0) {
      *(init + nc)= 0;
      FILE *file = fopen("/tmp/re.txt", "w");
      if (file == NULL) {
          perror("Error redirecting stdout");
          return;
      }

      printf("This will be written to the file.\n");
      fprintf(file, "%s", init);
      fclose(file);
      free(init);
    }
#endif
    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();


  /* Initialize the Decode*/
  init_decode();
}
