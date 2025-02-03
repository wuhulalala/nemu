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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <stdint.h>
#include <elf.h>

#ifdef CONFIG_FTRACE
extern Elf32_Ehdr elf_header;
extern Elf32_Shdr symtab;
extern Elf32_Shdr strtab;
extern Elf32_Sym sym_table[256];
extern char str_table[10000];
size_t indent = 0;
static char *p = NULL;
char *init = NULL;
uint64_t nc = 0;
static uint64_t max = 0;
#define THRESHOLD 100
#endif
#ifdef CONFIG_DIFFTEST
void difftest_skip_ref();
#endif

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S, TYPE_J, TYPE_B, TYPE_R,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immJ() do { *imm = ((SEXT(BITS(i, 31, 31), 1) << 19) | BITS(i, 19, 12) << 11 | BITS(i, 20, 20) << 10 | BITS(i, 30, 21)) << 1;} while(0)
#define immB() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 11 | BITS(i, 7, 7) << 10 | BITS(i, 30, 25) << 4 | BITS(i, 11, 8)) << 1;} while(0)
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_J:                   immJ(); break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    case TYPE_R: src1R(); src2R();         break;
    case TYPE_N: break;
    default: panic("unsupported type = %d", type);
  }
}

static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  int rd = 0; \
  word_t src1 = 0, src2 = 0, imm = 0; \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  INSTPAT_START();
  // U type
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui  , U, R(rd) = imm);

  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));

  // J type
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, {R(rd) = s->snpc; s->dnpc = s->pc + imm;
#ifdef CONFIG_FTRACE
    if (p == NULL) {
      init = p = (char *)malloc(1000);
      max = 1000;
    } else if (max - nc < THRESHOLD) {
      max = max * 2;
      char * t = (char*)malloc(max);
      memcpy(t, init, nc);
      free(init);
      init = t;
      p = init + nc;
    }
    size_t entnum = symtab.sh_size / symtab.sh_entsize;
    size_t n;
    for (int i = 0; i < entnum; i++) {
    Elf32_Sym *sym = &sym_table[i];
    if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC && (s->dnpc == sym->st_value)) {
      n = sprintf(p, "0x%x: ", s->pc);
      p += n;
      nc += n;
      for (int j = 0; j < indent; j++) {
        n = sprintf(p, "  ");
        p += n;
        nc += n;
      }
      n = sprintf(p, "call [%s@0x%x]\n", &str_table[sym->st_name], s->dnpc);
      p += n;
      nc += n;
      indent++;
    }
  }
#endif
  });
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, {R(rd) = s->snpc; s->dnpc = src1 + imm;
#ifdef CONFIG_FTRACE
    if (p == NULL) {
      init = p = (char *)malloc(1000);
      max = 1000;
    } else if (max - nc < THRESHOLD) {
      max = max * 2;
      char * t = (char*)malloc(max);
      memcpy(t, init, nc);
      free(init);
      init = t;
      p = init + nc;
    }
    size_t n;
    size_t entnum = symtab.sh_size / symtab.sh_entsize;
    if (BITS(s->isa.inst, 19, 15) == 1) {
      for (int i = 0; i < entnum; i++) {
        Elf32_Sym *sym = &sym_table[i];
        if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC && (s->pc >= sym->st_value && s->pc < sym->st_value + sym->st_size)) {
          //printf("%s\n", &str_table[sym->st_name]);
          indent--;
          n = sprintf(p, "0x%x: ", s->pc);
          p += n;
          nc += n;

          for (int j = 0; j < indent; j++) {
            n = sprintf(p, "  ");
            p += n;
            nc += n;
          }
          n = sprintf(p, "ret  [%s]\n", &str_table[sym->st_name]);
          p += n;
          nc += n;
        } 
      }
    } else {
      for (int i = 0; i < entnum; i++) {
        Elf32_Sym *sym = &sym_table[i];
        if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC && (s->dnpc == sym->st_value)) {
          n = sprintf(p, "0x%x: ", s->pc);
          p += n;
          nc += n;
          for (int j = 0; j < indent; j++) {
            n = sprintf(p, "  ");
            p += n;
            nc += n;
          }
          n = sprintf(p, "call [%s@0x%x]\n", &str_table[sym->st_name], s->dnpc);
          p += n;
          nc += n;
          indent++;
        }

      }
    }
#endif

  });

  // S type
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));
  INSTPAT("??????? ????? ????? 011 ????? 01000 11", sd     , S, Mw(src1 + imm, 8, src2));


  // B type
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq     , B, s->dnpc = (src1 == src2 ? s->pc + imm : s->snpc));
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne     , B, s->dnpc = (src1 != src2 ? s->pc + imm : s->snpc));
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge     , B, s->dnpc = ((sword_t)src1 >= (sword_t)src2 ? s->pc + imm : s->snpc));
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu    , B, s->dnpc = (src1 >= src2 ? s->pc + imm : s->snpc));
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt     , B, s->dnpc = ((sword_t)src1 < (sword_t)src2 ? s->pc + imm : s->snpc));
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu    , B, s->dnpc = (src1 < src2 ? s->pc + imm : s->snpc));

  // I type
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi    , I, R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb      , I, R(rd) = (int8_t)Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu     , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh      , I, R(rd) = (int16_t)Mr(src1 + imm, 2));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu     , I, R(rd) = Mr(src1 + imm, 2));
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw      , I, R(rd) = (int32_t)Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld      , I, R(rd) = (int64_t)Mr(src1 + imm, 8));
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti    , I, R(rd) = ((sword_t)src1 < (sword_t)imm) ? 1 : 0);
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu   , I, R(rd) = (src1 < imm) ? 1 : 0);
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli    , I, R(rd) = src1 << (0x0000001f & imm));
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli    , I, R(rd) = src1 >> (0x0000001f & imm));
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi    , I, R(rd) = src1 & imm);
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai    , I, R(rd) = (sword_t)src1 >> (0x0000001f & imm););
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori    , I, R(rd) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori     , I, R(rd) = src1 | imm);



  // R type
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add      , R, R(rd) = (sword_t)src1 + (sword_t)src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub      , R, R(rd) = (sword_t)src1 - (sword_t)src2);
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra      , R, R(rd) = (sword_t)src1 >> (0x0000001f & src2));
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll      , R, R(rd) = src1 << (0x0000001f & src2));
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl      , R, R(rd) = src1 >> (0x0000001f & src2));
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra      , R, R(rd) = (sword_t)src1 >> (0x0000001f & src2));
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt      , R, R(rd) = ((sword_t)src1 < (sword_t)src2) ? 1 : 0);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu     , R, R(rd) = (src1 < src2) ? 1 : 0);

  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul      , R, R(rd) = (int64_t)src1 * (int64_t)src2);
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh     , R, R(rd) = (int64_t)(sword_t)src1 * (int64_t)(sword_t)src2 >> 32);
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu    , R, R(rd) = (uint64_t)src1 * (uint64_t)src2 >> 32);

  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div      , R, R(rd) = (sword_t)src1 / (sword_t)src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu     , R, R(rd) = src1 / src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem      , R, R(rd) = (sword_t)src1 % (sword_t)src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu     , R, R(rd) = src1 % src2);

  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and      , R, R(rd) = src1 & src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or      , R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor      , R, R(rd) = src1 ^ src2);



  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10)); IFDEF(CONFIG_DIFFTEST, difftest_skip_ref();)); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));


  INSTPAT_END();
  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
