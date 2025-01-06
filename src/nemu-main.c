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
#include <time.h>
#include <stdlib.h>
#include "monitor/sdb/sdb.h"

void init_monitor(int, char *[]);
void init_sdb();
void am_init_monitor();
void engine_start();
int is_exit_status_bad();
void init_wp_pool();
void free_wp(WP* wp);
WP* new_wp();
struct watchpoint;
int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  /* Start engine. */
  engine_start();
  return is_exit_status_bad();



  // init_sdb();
  // init_wp_pool();
  // int seed = time(0);
  // srand(seed);
  // int round = atoi(argv[1]);
  // unsigned n[32];
  // WP *p[32];
  // int count = 32;
  // while (count > 0) {
    // for (int i = 0; i < round; i++) {
      // char str[256];
      // n[i] = rand();
      // sprintf(str, "%u", n[i]);
      // if (i == 0) {
        // printf("%u=====\n", n[i]);
      // } else {
        // printf("%u\n", n[i]);
      // }
      // p[i] = new_wp(str);
    // }

    // for (int i = 0; i < round - 1; i++) {
      // if (i == round - 1) {
        // printf("%u=====\n", n[i]);
      // } else {
        // printf("%u\n", n[i]);
      // }
      // Assert(p[i]->value == n[i], "not equal");
      // free_wp(p[i]);
    // }
    // count--;
    
  // }

}
