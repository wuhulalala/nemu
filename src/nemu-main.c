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

#include <common.h>
#include <stdio.h>

void init_monitor(int, char *[]);
void init_sdb();
void am_init_monitor();
void engine_start();
word_t expr(char *e, bool *success);
int is_exit_status_bad();

int main(int argc, char *argv[]) {
  init_sdb();
  char buf[65536];
  FILE *fp = fopen("/home/liscopye/ics2024/nemu/src/output.txt", "r");
  Assert(fp != NULL, "can not open file");
  while (fgets(buf, sizeof(buf), fp)) {
    buf[strcspn(buf, "\n")] = '\0';
    uint32_t res;
    sscanf(buf, "%u", &res);
    strtok(buf, " ");
    char *e = strtok(NULL , " ");
    bool success;
    printf("%u %s\n", res, e);
    uint32_t result = (uint32_t)expr(e, &success);
    Assert(result == res, "result: %u, expect: %u\n the expression is %s", result, res, e);
  }
  fclose(fp);
  ///* Initialize the monitor. */
//#ifdef CONFIG_TARGET_AM
  //am_init_monitor();
//#else
  //init_monitor(argc, argv);
//#endif

  ///* Start engine. */
  //engine_start();
  ////return is_exit_status_bad();
  return 0;
}
