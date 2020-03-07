// See LICENSE for license details.

#include "insn_template.h"

reg_t rv32_sbreak(processor_t* p, insn_t insn, reg_t pc)
{
  int xlen = 32;
  reg_t npc = sext_xlen(pc + insn_length( MATCH_SBREAK));
  #include "insns/sbreak.h"
  return npc;
}

reg_t rv64_sbreak(processor_t* p, insn_t insn, reg_t pc)
{
  int xlen = 64;
  reg_t npc = sext_xlen(pc + insn_length( MATCH_SBREAK));
  #include "insns/sbreak.h"
  return npc;
}
