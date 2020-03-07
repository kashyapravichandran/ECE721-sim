// See LICENSE for license details.

#include "insn_template.h"

reg_t rv32_fmv_x_s(processor_t* p, insn_t insn, reg_t pc)
{
  int xlen = 32;
  reg_t npc = sext_xlen(pc + insn_length( MATCH_FMV_X_S));
  #include "insns/fmv_x_s.h"
  return npc;
}

reg_t rv64_fmv_x_s(processor_t* p, insn_t insn, reg_t pc)
{
  int xlen = 64;
  reg_t npc = sext_xlen(pc + insn_length( MATCH_FMV_X_S));
  #include "insns/fmv_x_s.h"
  return npc;
}
