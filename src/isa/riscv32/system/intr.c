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

#define CAUSE_USER_ECALL 8
#define CAUSE_SUPERVISOR_ECALL 9
#define CAUSE_MACHINE_ECALL 11
#define CAUSE_LOAD_PAGE_FAULT 13

#define PRV_U 0
#define PRV_S 1
#define PRV_M 3

#ifdef CONFIG_ETRACE
typedef struct {
    uint32_t seqnr;        
    vaddr_t pre_pc;        
    vaddr_t cur_pc;        
    
    uint32_t mcause;       
    uint32_t pre_mode;     
    uint32_t cur_mode;    
    vaddr_t mepc;         
    
    uint32_t mstatus;     
    word_t   a7;          
} Etrace_Entry;

static void log_exception();
static void print_etrace(Etrace_Entry *e);
static const char* get_mcause_str(uint32_t mcause);  
#endif

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
  cpu.mepc = epc;
  cpu.mcause = NO;
  cpu.mstatus.fields.MPIE = cpu.mstatus.fields.MIE;
  cpu.mstatus.fields.MIE = 0;
  cpu.mstatus.fields.MPP = PRV_M;
  Assert(cpu.mtvec, "mtvec is NULL");
#ifdef CONFIG_ETRACE
  log_exception();
#endif
  return cpu.mtvec;
}

word_t isa_mret_intr() {
  cpu.mstatus.fields.MIE = cpu.mstatus.fields.MPIE;
  cpu.mstatus.fields.MPIE = 1;
  cpu.mstatus.fields.MPP = 0;
  return cpu.mepc;
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}

#ifdef CONFIG_ETRACE
static void print_etrace(Etrace_Entry *e) {
    const char *cause_str = get_mcause_str(e->mcause);
    
    printf("[etrace] #%d  %s @ 0x%x -> 0x%x",
           e->seqnr, cause_str, e->pre_pc, e->cur_pc);
    
    printf(" (mepc=0x%x", e->mepc);
    
    if (e->mcause == CAUSE_USER_ECALL || 
        e->mcause == CAUSE_SUPERVISOR_ECALL || 
        e->mcause == CAUSE_MACHINE_ECALL) {
        printf(", a7=%d", e->a7);
    }
    printf(")\n");
    
    printf("         pre_mode=%d, cur_mode=%d, mstatus=0x%x\n",
           e->pre_mode, e->cur_mode, e->mstatus);
}

static const char* get_mcause_str(uint32_t mcause) {
    switch (mcause) {
        case CAUSE_USER_ECALL:
            return "U-ecall";
        case CAUSE_SUPERVISOR_ECALL:
            return "S-ecall";
        case CAUSE_MACHINE_ECALL:
            return "M-ecall";
        case CAUSE_LOAD_PAGE_FAULT:
            return "Page Fault";
        default:
            return "Unknown";
    }
}

static void log_exception() {
    static uint32_t seqnr = 0;
    Etrace_Entry e = {
        .seqnr = ++seqnr,
        .pre_pc = cpu.pc,
        .cur_pc = cpu.mtvec,
        .mcause = cpu.mcause,
        .pre_mode = cpu.mstatus.fields.MPP,
        .cur_mode = PRV_M,  // 异常后进入M-mode
        .mepc = cpu.mepc,
        .mstatus = cpu.mstatus.val,
        .a7 = cpu.gpr[17]   // a0寄存器值
    };
    
    print_etrace(&e);
}

#endif