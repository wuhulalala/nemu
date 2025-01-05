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
#define Assert(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "Assertion failed: (%s), file %s, line %d, function %s\n", \
                    #expr, __FILE__, __LINE__, __func__); \
            abort(); \
        } \
    } while (0)

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define MAXDEPTH 30
// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *position;
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static int choose(uint32_t n) {
  if (n == 0) {
      return 0;  
    }
  return rand() % n;
}

static void gen_num() {
  uint32_t rand_num = (uint32_t)rand(); 
  int size = sprintf(position, "%u", rand_num);
  // printf("the position is %p, the size is %d\n", position, size);
  position = position + size;
  // printf("the position is %p\n", position);
}

static void gen(char ch) {
  int size = sprintf(position, "%c", ch);
  // printf("the position is %p, the size is %d\n", position, size);
  position = position + size;
  // printf("the position is %p\n", position);
}

static void gen_rand_op() {
  int n = choose(4);
  int size = 0;
  switch (n) {
    case 0:
      size = sprintf(position, "%c", '+');
      break;
    case 1:
      size = sprintf(position, "%c", '-');
      break;
    case 2:
      size = sprintf(position, "%c", '*');
      break;
    default:
      size = sprintf(position, "%c", '/');
      break;
  } 
  // printf("the position is %p, the size is %d\n", position, size);
  position = position + size;
  // printf("the position is %p, the buf is %p\n", position, buf);
}



static void gen_rand_expr(int depth) {
  if (depth >= MAXDEPTH) {
    gen_num();
    return;
  }
  switch (choose(3)) {
    case 0: gen_num(); break;
    case 1: gen('('); gen_rand_expr(depth + 1); gen(')'); break;
    default: gen_rand_expr(depth + 1); gen_rand_op(); gen_rand_expr(depth + 1); break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    memset(buf, 0, 65536);
    position = buf;
    gen_rand_expr(0);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc -w /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    char buffer[128] = {0};
    long pos = ftell(fp);
    fscanf(fp, "%s", buffer);
    if (strcmp("", buffer) == 0) {
      pclose(fp);
      continue;
    } else {
      uint32_t result;
      sscanf(buffer, "%u", &result);
      printf("%u %s\n", result, buf);
    }
  }
  return 0;
}
