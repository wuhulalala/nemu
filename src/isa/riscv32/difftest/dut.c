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
#include <cpu/difftest.h>
#include "../local-include/reg.h"
static bool enable_csr_difftest = false;
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc, vaddr_t npc) {
  bool flag = true;
  for (int i = 0; i < MUXDEF(CONFIG_RVE, 16, 32); i++) {
    if (ref_r->gpr[i] != cpu.gpr[i]) {
      printf("reg %s value mismatch: ref = %#x, dut = %#x, pc = %#x\n", reg_name(i), ref_r->gpr[i], cpu.gpr[i], pc);
      flag = false;
    }
  }

  if (cpu.mstatus.val == 0x1800) {
    enable_csr_difftest = true;
  }

  if (enable_csr_difftest) {
    if (ref_r->mstatus.val != cpu.mstatus.val) {
      printf("mstatus value mismatch: ref = %#x, dut = %#x, pc = %#x\n", ref_r->mstatus.val, cpu.mstatus.val, pc);
      //flag = false;
    }

    if (ref_r->mepc != cpu.mepc) {
      printf("mpec value mismatch: ref = %#x, dut = %#x, pc = %#x\n", ref_r->mepc, cpu.mepc, pc);
    }


  }

  if (ref_r->pc != npc) {
    printf("ref pc = %#x  dut pc = %#x\n", ref_r->pc, npc);
    printf("csr registers:\n");
    //printf("mstatus = %#x\n", cpu.mstatus.val);
    printf("mtvec = %#x\n", cpu.mtvec);
    printf("mepc = %#x\n", cpu.mepc);
    printf("mcause = %#x\n", cpu.mcause);
    flag = false;
  }
  
  return flag;
}

void isa_difftest_attach() {
}
