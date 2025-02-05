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

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>

// 按功能分组的位域定义
struct mstatus_fields {
    // 中断控制组 [3:0]
    struct {
        uint32_t UIE: 1;    // U-mode 中断使能
        uint32_t SIE: 1;    // S-mode 中断使能
        uint32_t Reserved0: 1;
        uint32_t MIE: 1;    // M-mode 中断使能
    } interrupt_enable;
    
    // 中断状态保存组 [7:4]
    struct {
        uint32_t UPIE: 1;   // U-mode 之前的中断状态
        uint32_t SPIE: 1;   // S-mode 之前的中断状态
        uint32_t Reserved1: 1;
        uint32_t MPIE: 1;   // M-mode 之前的中断状态
    } previous_interrupt;
    
    // 特权级控制组 [12:8]
    struct {
        uint32_t SPP: 1;    // S-mode 之前的特权级
        uint32_t Reserved2: 2;
        uint32_t MPP: 2;    // M-mode 之前的特权级
    } privilege_control;
    
    // 状态控制组 [16:13]
    struct {
        uint32_t FS: 2;     // 浮点状态
        uint32_t XS: 2;     // 扩展状态
    } state_control;
    
    // 内存访问控制组 [22:17]
    struct {
        uint32_t MPRV: 1;   // 修改特权级
        uint32_t SUM: 1;    // S-mode 访问 U-mode 内存
        uint32_t MXR: 1;    // 使可执行页可读
        uint32_t TVM: 1;    // 虚拟内存陷阱
        uint32_t TW: 1;     // WFI 指令陷阱
        uint32_t TSR: 1;    // SRET 指令陷阱
    } memory_control;
    
    // 保留和状态位 [31:23]
    struct {
        uint32_t Reserved3: 8;
        uint32_t SD: 1;     // 状态脏位
    } status;
};

typedef union {
  struct mstatus_fields fields;
  word_t val;
} MSTATUS;


typedef struct {
    MSTATUS mstatus;    // Machine Status Register
    word_t mtvec;      // Machine Trap-Vector Base Address Register
    word_t mepc;       // Machine Exception Program Counter
    word_t mcause;     // Machine Cause Register
    word_t mie;        // Machine Interrupt Enable Register
    word_t mscratch;   // Machine Scratch Register
    word_t mip;        // Machine Interrupt Pending Register
    word_t mtime;      // Timer (假设模拟一个时间寄存器)
    word_t mcycle;     // Cycle Count (假设模拟一个周期计数器)
} CSR;


typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  vaddr_t pc;
  CSR csr;
  uint32_t mode;   
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// decode
typedef struct {
  uint32_t inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

#endif
