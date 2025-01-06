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

#include "sdb.h"





static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}


WP* new_wp(char *str) {
  Assert(free_ != NULL, "There is no free watch point");
  WP *w = free_;
  free_ = free_ -> next;
  strcpy(w->expr, str);
  if (strcmp(w->expr, str) != 0) {
    Assert(0, "fail to copy the expression");
  }
  bool success;
  w->value = expr(str, &success);
  if (!success) {
    Log("wrong expression");
    return NULL;
  }
  if (head == NULL) {
    head = w;
    head->next = NULL;
    return head;
  }

  w->next = head;
  head = w;
  return w;
}

void free_wp(WP *wp) {
  Assert(wp, "wp is null");
  WP *prev = NULL, *cur = head;
  Assert(cur, "head is null");
  while (cur != NULL && cur != wp) {
    prev = cur;
    cur = cur->next;
  }

  Assert(cur, "no wp");
  if (prev == NULL) {
    head = NULL;
    wp->next = free_;
    free_ = wp;
    return;
  }

  prev->next = wp->next;
  if (free_ == NULL) {
    free_ = wp;
    free_->next = NULL;
    return;
  }


  wp->next = free_;
  free_ = wp;


}

WP* number2addr(int n) {
  Assert(n >= 0 && n < NR_WP, "expect: n >=0 and n < %d", NR_WP);
  return &wp_pool[n];

}


void watchpoint_display() {
  WP* wp = head;
  if (wp == NULL) {
    printf("No watchpoints\n");
  }
  while (wp != NULL) {
    printf("Number: %d\texpression: %s\n", wp->NO, wp->expr);
    wp = wp->next;
  }
  return;
}


bool watchpoint_diff() {
  WP *wp = head;
  bool changed = false;
  while (wp != NULL) {
    bool success;
    uint64_t result = expr(wp->expr, &success);
    Assert(success, "Wrong, can not eval the watchpoint expression's value");
    if (wp->value != result) {
      changed = true;
      printf("Number: %d\texpression: %s\n", wp->NO, wp->expr);
      printf("Old value: %lu\n", wp->value);
      printf("New value: %lu\n", result);
      wp->value = result;
    }

    wp = wp->next; 


  }
  return changed;
}
/* TODO: Implement the functionality of watchpoint */

