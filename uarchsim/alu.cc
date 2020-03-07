#include <cmath>
#include "pipeline.h"
#include "trap.h"
#include "mulhi.h"
#include "softfloat.h"
#include "platform.h" // softfloat isNaNF32UI, etc.
#include "internals.h" // ditto

// Helper functions at the bottom of this file.
static void ExecMULT(reg_t, reg_t,
                     reg_t*, reg_t*);
static void ExecMULTU(reg_t, reg_t,
                      reg_t*, reg_t*);
static void ExecSRA(reg_t, unsigned int, reg_t*);
static void ExecSRAV(reg_t, reg_t, reg_t*);


void pipeline_t::agen(unsigned int index) {
	insn_t inst;
	reg_t addr;

	// Only loads and stores should do AGEN.
	assert(IS_MEM_OP(PAY.buf[index].flags));

	inst = PAY.buf[index].inst;

	// Execute the AGEN.
  if(IS_AMO(PAY.buf[index].flags)){
    //AMO ops do not use the displacement addressing
  	addr = PAY.buf[index].A_value.dw;
  } else if(IS_LOAD(PAY.buf[index].flags)){
    //Loads use the I-type immediate encoding
  	addr = PAY.buf[index].A_value.dw + inst.i_imm();
  } else {
    //Stores use the S-type immediate encoding
  	addr = PAY.buf[index].A_value.dw + inst.s_imm();
  }
	PAY.buf[index].addr = addr;

	//// Adjust address of the lower half of DLW and DSW.
	//if ((inst.opcode() == DLW) || (inst.opcode() == DSW)) {
	//	assert(PAY.buf[index].split);
	//	if (!PAY.buf[index].upper) {
	//		PAY.buf[index].addr += 4;
	//	}
	//}
}	// agen()


void pipeline_t::alu(unsigned int index) {

	insn_t inst;
	insn_t insn;
	//reg_t hi, lo;

	//unsigned int x,y;
	//DOUBLE_WORD a, b, r;

	inst = PAY.buf[index].inst;
  insn = insn; //Required for macors
  pipeline_t* p = this;    //Required for macros
	db_t* actual;

	switch (inst.opcode()) {

//
// LOAD/STORE instructions
//

    case OP_LOAD:
    case OP_STORE:
    case OP_LOAD_FP:
    case OP_STORE_FP:
			// Loads and stores are executed by memory interface.
			assert(0);
			break;

//
// control transfer operations
//


		case OP_JAL:
			PAY.buf[index].C_value.dw = INCREMENT_PC(PAY.buf[index].pc) ;
			PAY.buf[index].c_next_pc = (PAY.buf[index].pc + inst.uj_imm());  //uj_imm is already sign extended
			break;

		case OP_JALR:
			PAY.buf[index].C_value.dw = INCREMENT_PC(PAY.buf[index].pc) ;
			PAY.buf[index].c_next_pc = ((PAY.buf[index].A_value.dw + inst.i_imm()) & ~reg_t(1)); // Zeroes the LSB bit as per ISA
			break;

    case OP_BRANCH:
      switch(inst.funct3()) {
    		case FN3_BEQ:
    			PAY.buf[index].c_next_pc = (PAY.buf[index].A_value.dw == PAY.buf[index].B_value.dw) ?
                            			    PAY.buf[index].pc + inst.sb_imm() : //sb_imm is already a multiple of 2 PC target
                                      INCREMENT_PC(PAY.buf[index].pc);
    			break;
    
    		case FN3_BNE:
    			PAY.buf[index].c_next_pc = (PAY.buf[index].A_value.dw != PAY.buf[index].B_value.dw) ?
                            			    PAY.buf[index].pc + inst.sb_imm() : //sb_imm is already a multiple of 2 PC target
                                      INCREMENT_PC(PAY.buf[index].pc);
    			break;
    
    		case FN3_BLT:
    			PAY.buf[index].c_next_pc = (PAY.buf[index].A_value.sdw < PAY.buf[index].B_value.sdw) ? //Use signed values
                            			    PAY.buf[index].pc + inst.sb_imm() : //sb_imm is already a multiple of 2 PC target
                                      INCREMENT_PC(PAY.buf[index].pc);
    			break;
    
    		case FN3_BGE:
    			PAY.buf[index].c_next_pc = (PAY.buf[index].A_value.sdw >= PAY.buf[index].B_value.sdw) ? //Use signed values
                            			    PAY.buf[index].pc + inst.sb_imm() : //sb_imm is already a multiple of 2 PC target
                                      INCREMENT_PC(PAY.buf[index].pc);
    			break;
    
    		case FN3_BLTU:
    			PAY.buf[index].c_next_pc = (PAY.buf[index].A_value.dw < PAY.buf[index].B_value.dw) ? //Use unsigned values
                            			    PAY.buf[index].pc + inst.sb_imm() : //sb_imm is already a multiple of 2 PC target
                                      INCREMENT_PC(PAY.buf[index].pc);
    			break;
    
    		case FN3_BGEU:
    			PAY.buf[index].c_next_pc = (PAY.buf[index].A_value.dw >= PAY.buf[index].B_value.dw) ? //Use unsigned values
                            			    PAY.buf[index].pc + inst.sb_imm() : //sb_imm is already a multiple of 2 PC target
                                      INCREMENT_PC(PAY.buf[index].pc);
    			break;
      }
      break; //OP_BRANCH

//
// Integer ALU operations
//

    case OP_OP_IMM:
      switch(inst.funct3()){
        //ADDI
        case FN3_ADD_SUB:
    	    PAY.buf[index].C_value.sdw = PAY.buf[index].A_value.sdw + inst.i_imm();
          //// Check if this is NOP with a fetch exception.
          //// If so, throw the appropriate exception
          //if(PAY.buf[index].fetch_exception){
          //  if(PAY.buf[index].fetch_exception_cause == CAUSE_MISALIGNED_FETCH){
          //    throw new trap_instruction_address_misaligned(PAY.buf[index].pc);
          //  } else if(PAY.buf[index].fetch_exception_cause == CAUSE_FAULT_FETCH){
          //    throw new trap_instruction_access_fault(PAY.buf[index].pc);
          //  }
          //}
          break;
        //SLTI
        case FN3_SLT:
    	    PAY.buf[index].C_value.dw = PAY.buf[index].A_value.sdw < sreg_t(inst.i_imm()) ? reg_t(1) : reg_t(0);
          break;
        //SLTIU
        case FN3_SLTU:
    	    PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw < reg_t(inst.i_imm()) ? reg_t(1) : reg_t(0);
          break;
        //XORI
        case FN3_XOR:
    	    PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw ^ inst.i_imm();
          break;
        //ORI 
        case FN3_OR:
    	    PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw | inst.i_imm();
          break;
        //ANDI
        case FN3_AND:
    	    PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw & inst.i_imm();
          break;
        //SLLI
        case FN3_SLL:
    	    PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw << inst.shamt();
          break;
        case FN3_SR:
          //SRLI
          //LSB masked to be 1'b0 as it is part of shamt in 64bit encoding
          // Logical right shift, hence using unsigned operands
          if((inst.funct7() & 0x7e) == FN7_SRL){ 
    	    	PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw >> inst.shamt();
          } 
          //SRAI
          //LSB masked to be 1'b0 as it is part of shamt in 64bit encoding
          // Arithmatic right shift, hence using signed operands
          else if((inst.funct7() & 0x7e) == FN7_SRA){
    	    	PAY.buf[index].C_value.sdw = PAY.buf[index].A_value.sdw >> inst.shamt();
          }
          //Undefined instruction
          else{
            assert(0); //Undefined instruction
          }
          break;

      }
      break; //OP_OP_IMM
    case OP_OP_IMM_32:
      switch(inst.funct3()){
        //ADDIW
        case FN3_ADD_SUB:
          //sext32 truncates to 32 bit and then sign extends to 64 bit as per the ISA requirement
    	   	PAY.buf[index].C_value.sdw = sext32(PAY.buf[index].A_value.sdw + inst.i_imm());
          break;
        //SLLIW
        case FN3_SLL:
    	   	PAY.buf[index].C_value.sdw = sext32(PAY.buf[index].A_value.dw << (inst.shamt() & 0x1f));
          break;
        case FN3_SR:
          //SRLIW
          // Logical right shift, hence using unsigned operands
          if(inst.funct7() == FN7_SRL){  
    	    	PAY.buf[index].C_value.sdw = sext32(PAY.buf[index].A_value.w[0] >> (inst.shamt() & 0x1f));
          } 
          //SRAIW
          // Arithmatic right shift, hence using signed operands
          else if(inst.funct7() == FN7_SRA){
    	    	PAY.buf[index].C_value.sdw = sext32((int32_t)PAY.buf[index].A_value.w[0] >> (inst.shamt() & 0x1f));
          }
          //Undefined instruction
          else{
            assert(0); 
          }
          break;
      }
      break; //OP_OP_IMM_32
    case OP_OP:
      if(inst.funct7() == FN7_MULDIV){
        switch(inst.funct3()){
          //MUL
 	  	    case FN3_MUL:
    	    	PAY.buf[index].C_value.sdw = sreg_t(PAY.buf[index].A_value.dw * PAY.buf[index].B_value.dw);
    	    	break;
          //MULH
    	    case FN3_MULH:
    	    	PAY.buf[index].C_value.sdw = mulh(PAY.buf[index].A_value.dw,PAY.buf[index].B_value.dw);
    	    	break;
          //MULHSU
 	  	    case FN3_MULHSU:
    	    	PAY.buf[index].C_value.sdw = mulhsu(PAY.buf[index].A_value.dw,PAY.buf[index].B_value.dw);
          //MULHU
    	    case FN3_MULHU:
    	    	PAY.buf[index].C_value.sdw = mulhu(PAY.buf[index].A_value.dw,PAY.buf[index].B_value.dw);
    	    	break;
          //DIV
    	    case FN3_DIV:
            {
              sreg_t lhs = PAY.buf[index].A_value.sdw;
              sreg_t rhs = PAY.buf[index].B_value.sdw; 
              if(rhs == 0)
                PAY.buf[index].C_value.dw = UINT64_MAX;
              else if(lhs == INT64_MIN && rhs == -1)
                PAY.buf[index].C_value.sdw = lhs;
              else
                PAY.buf[index].C_value.sdw = lhs / rhs;
            }
    	    	break;
          //DIVU
    	    case FN3_DIVU:
            {
              reg_t lhs = PAY.buf[index].A_value.dw;
              reg_t rhs = PAY.buf[index].B_value.dw; 
              if(rhs == 0)
                PAY.buf[index].C_value.dw = UINT64_MAX;
              else
                PAY.buf[index].C_value.sdw = sreg_t(lhs / rhs);
            }
    	    	break;
          //REM
    	    case FN3_REM:
            {
              sreg_t lhs = PAY.buf[index].A_value.sdw;
              sreg_t rhs = PAY.buf[index].B_value.sdw; 
              if(rhs == 0)
                PAY.buf[index].C_value.sdw = lhs;
              else if(lhs == INT64_MIN && rhs == -1)
                PAY.buf[index].C_value.sdw = 0;
              else
                PAY.buf[index].C_value.sdw = lhs % rhs;
            }
            break;
          //REMU
    	    case FN3_REMU:
            {
              reg_t lhs = PAY.buf[index].A_value.dw;
              reg_t rhs = PAY.buf[index].B_value.dw; 
              if(rhs == 0)
                PAY.buf[index].C_value.sdw = PAY.buf[index].A_value.sdw;
              else
                PAY.buf[index].C_value.sdw = sreg_t(lhs % rhs);
            }
    	    	break;
        }
      }
      else {
        switch(inst.funct3()){
    	  	case FN3_ADD_SUB:
            //ADD
            if(inst.funct7() == FN7_ADD){  
    	  		  PAY.buf[index].C_value.sdw = PAY.buf[index].A_value.sdw + PAY.buf[index].B_value.sdw ;
            } 
            //SUB
            else if(inst.funct7() == FN7_SUB){
    	  		  PAY.buf[index].C_value.sdw = PAY.buf[index].A_value.sdw - PAY.buf[index].B_value.sdw ;
            }
            //Undefined instruction
            else{
              assert(0); 
            }
    	  		break;
          //SLL
    	  	case FN3_SLL:
    	  		PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw << (PAY.buf[index].B_value.dw & 0x3f) ;
    	  		break;
          //SLT
    	  	case FN3_SLT:
    	  		PAY.buf[index].C_value.dw = (PAY.buf[index].A_value.sdw < PAY.buf[index].B_value.sdw) ? reg_t(1) : reg_t(0) ;
    	  		break;
          //SLTU
    	  	case FN3_SLTU:
    	  		PAY.buf[index].C_value.dw = (PAY.buf[index].A_value.dw) < (PAY.buf[index].B_value.dw) ? reg_t(1) : reg_t(0) ;
    	  		break;
          //XOR
    	  	case FN3_XOR:
    	  		PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw ^ PAY.buf[index].B_value.dw;
    	  		break;
          case FN3_SR:
            //SRL
            // Logical right shift, hence using unsigned operands
            if(inst.funct7() == FN7_SRL){  
    	  		  PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw >> (PAY.buf[index].B_value.dw & 0x3f);
            } 
            //SRA
            // Arithmetic right shift, hence using signed operands
            else if(inst.funct7() == FN7_SRA){
    	  		  PAY.buf[index].C_value.sdw = PAY.buf[index].A_value.sdw >> (PAY.buf[index].B_value.dw & 0x3f);
            }
            //Undefined instruction
            else{
              assert(0); 
            }
          break;
          //OR
    	  	case FN3_OR:
    	  		PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw | PAY.buf[index].B_value.dw;
    	  		break;
          //AND
       	  case FN3_AND:
    	  		PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw & PAY.buf[index].B_value.dw;
    	  		break;
        }
      }
      break;//OP_OP

    //TODO: Add correct functionality
    case OP_OP_32:
      if(inst.funct7() == FN7_MULDIV){
        switch(inst.funct3()){
          //MULW
 	  	    case FN3_MUL:
    	    	PAY.buf[index].C_value.sdw = sext32(PAY.buf[index].A_value.dw * PAY.buf[index].B_value.dw);
    	    	break;
          //DIVW
    	    case FN3_DIV:
            {
              sreg_t lhs = sext32(PAY.buf[index].A_value.sdw);
              sreg_t rhs = sext32(PAY.buf[index].B_value.sdw); 
              if(rhs == 0)
                PAY.buf[index].C_value.dw = UINT64_MAX;
              else
                PAY.buf[index].C_value.sdw = sext32(lhs / rhs);
            }
    	    	break;
          //DIVUW
    	    case FN3_DIVU:
            {
              //zext32() truncates the operand to 32 bits and then 0 extends to 64 bit as per ISA requirement
              reg_t lhs = zext32(PAY.buf[index].A_value.dw);
              reg_t rhs = zext32(PAY.buf[index].B_value.dw); 
              if(rhs == 0)
                PAY.buf[index].C_value.dw = UINT64_MAX;
              else
                PAY.buf[index].C_value.sdw = sext32(lhs / rhs);
            }
    	    	break;
          //REMW
    	    case FN3_REM:
            {
              sreg_t lhs = sext32(PAY.buf[index].A_value.sdw);
              sreg_t rhs = sext32(PAY.buf[index].B_value.sdw); 
              if(rhs == 0)
                PAY.buf[index].C_value.sdw = lhs;
              else
                PAY.buf[index].C_value.sdw = sext32(lhs % rhs);
            }
            break;
          //REMUW
    	    case FN3_REMU:
            {
              reg_t lhs = zext32(PAY.buf[index].A_value.dw);
              reg_t rhs = zext32(PAY.buf[index].B_value.dw); 
              if(rhs == 0)
                PAY.buf[index].C_value.sdw = sext32(PAY.buf[index].A_value.sdw);
              else
                PAY.buf[index].C_value.sdw = sext32(lhs % rhs);
            }
    	    	break;
        }//switch(inst.funct3())
      }
      else {
        switch(inst.funct3()){
    	  	case FN3_ADD_SUB:
            //ADDW
            if(inst.funct7() == FN7_ADD){  
    	  		  PAY.buf[index].C_value.sdw = sext32(PAY.buf[index].A_value.sdw + PAY.buf[index].B_value.sdw) ;
            } 
            //SUBW
            else if(inst.funct7() == FN7_SUB){
    	  		  PAY.buf[index].C_value.sdw = sext32(PAY.buf[index].A_value.sdw - PAY.buf[index].B_value.sdw) ;
            }
            //Undefined instruction
            else{
              assert(0); 
            }
    	  		break;
          //SLLW
    	  	case FN3_SLL:
    	  		PAY.buf[index].C_value.dw = sext32(PAY.buf[index].A_value.w[0] << (PAY.buf[index].B_value.dw & 0x1f)) ;
    	  		break;
          case FN3_SR:
            //SRLW
            // Logical right shift, hence using unsigned operands
            if(inst.funct7() == FN7_SRL){  
    	  		  PAY.buf[index].C_value.dw = sext32(PAY.buf[index].A_value.w[0] >> (PAY.buf[index].B_value.dw & 0x1f));
            } 
            //SRAW
            // Arithmetic right shift, hence using signed operands
            else if(inst.funct7() == FN7_SRA){
    	  		  PAY.buf[index].C_value.sdw = sext32((int32_t)PAY.buf[index].A_value.w[0] >> (PAY.buf[index].B_value.dw & 0x1f));
            }
            //Undefined instruction
            else{
              assert(0); 
            }
          break;
        }
      }
      break;//OP_OP_32
//
// miscellaneous
//

    case OP_SYSTEM:
      //if(inst.funct3() != FN3_SC_SB){
      //  pipeline_t* p = this; // Needed for validate_csr macro to work 
      //  // This can throw privileged instruction exception if correct permissions
      //  // are missing. This is somewhat of a problem when instructions are executed
      //  // out of order. We do nothing about it here. Just catch the trap and continue.
      //  // Permissions will be checked again at retirement and that is when it really 
      //  // matters since that is when state is updated. 
      //  int csr = 0;
      //  //try{
      //    csr = validate_csr(PAY.buf[index].CSR_addr, (PAY.buf[index].A_log_reg !=0));
      //  //}
      //  //catch(trap_t& t){
      //  //  ifprintf(logging_on,execute_log,"Privileged CSR access trap thrown\n");
      //  //}
      //  reg_t old;

      //  try{
      //   old = get_pcr(csr);
      //  }
      //  catch(serialize_t& s) {
      //    // Undo the serialization that happened
      //    // as it will happen again at retire
      //    ifprintf(logging_on,stderr,"ALU: Caught serialize_t exception...........\n");
      //    serialize();
      //  }

      //  switch(inst.funct3()){
      //    case FN3_CLR:
      //      PAY.buf[index].CSR_new_value = old & ~PAY.buf[index].A_value.dw;
      //      break;
      //    case FN3_RW:
      //      old = get_pcr(csr);
      //      PAY.buf[index].CSR_new_value = PAY.buf[index].A_value.dw;
      //      break;
      //    case FN3_SET:
      //      csr = validate_csr(PAY.buf[index].CSR_addr, PAY.buf[index].A_value.dw != 0);
      //      old = get_pcr(csr);
      //      PAY.buf[index].CSR_new_value = old | PAY.buf[index].A_value.dw;
      //      break;
      //    case FN3_CLR_IMM:
      //      PAY.buf[index].CSR_new_value = old & ~(reg_t)PAY.buf[index].A_log_reg;
      //      break;
      //    case FN3_RW_IMM:
      //      PAY.buf[index].CSR_new_value = (reg_t)PAY.buf[index].A_log_reg;
      //      break;
      //    case FN3_SET_IMM:
      //      PAY.buf[index].CSR_new_value = old | (reg_t)PAY.buf[index].A_log_reg;
      //      break;
      //  }
      //  // Verify for atomicity furing retire by comparing the old value with the
      //  // current value of a CSR before writing to it. If they are not equal, then
      //  // an external agent must have modified a CSR. Redo this instruction in that case.
      //  PAY.buf[index].C_value.dw = old;
      //  // Must use "new" otherwise the exception behaves as a singleton and 
      //  // new exception deletes the old reference.
      //  throw new trap_csr_instruction();
      //} else {
      //  uint64_t funct12 = inst.funct12();
      //  if(funct12 == FN12_SRET){
      //    pipeline_t* p = this; // Needed for validate_csr and require_supervisor macros to work 
      //    // This is a macro defined in decode.h
      //    // This will throw a privileged_instruction trap if proessor not in supervisor mode.
      //    require_supervisor; 
      //    //int csr = validate_csr(CSR_STATUS, true);
      //    int csr = validate_csr(PAY.buf[index].CSR_addr, true);
      //    reg_t old = get_pcr(csr);
      //    PAY.buf[index].CSR_new_value  = ((old & ~(SR_S | SR_EI)) |
      //                                    ((old & SR_PS) ? SR_S : 0)) |
      //                                    ((old & SR_PEI) ? SR_EI : 0);
      //    // Verify for atomicity furing retire by comparing the old value with the
      //    // current value of a CSR before writing to it. If they are not equal, then
      //    // an external agent must have modified a CSR. Redo this instruction in that case.
      //    // TODO: Use CSR_old_value instead
      //    PAY.buf[index].C_value.dw = old;
      //    //PAY.buf[index].CSR_old_value = old;
      //    // Must use "new" otherwise the exception behaves as a singleton and 
      //    // new exception deletes the old reference.
      //    throw new trap_csr_instruction();

      //  }
      //  // SCALL and SBREAK
  	  //  // These skip the IQ and execution lanes (completed in Dispatch Stage).
      //  else{
		  //    assert(0);
      //  }
      //}
      break;//OP_SYSTEM

		case OP_LUI:
			PAY.buf[index].C_value.dw = inst.u_imm();
			break;

    case OP_AUIPC:
			PAY.buf[index].C_value.dw = PAY.buf[index].pc + inst.u_imm();
			break;

  #ifdef RISCV_MICRO_CHECKER
    // If checker is enabled, simply copy the result from the debug buffer
    case OP_MADD:
    case OP_MSUB:
    case OP_NMADD:
    case OP_NMSUB:
        // If the instruction was mapped to an invalid DB entry, actual
        // might be invalid. Must check for validity. This instruction will never commit
        // and need not have the correct result.
        if(valid_debug_index(PAY.buf[index].db_index)){
	        actual = pipe->peek(PAY.buf[index].db_index);		// Pointer to corresponding instruction in the functional simulator.
          //fprintf(stderr,"actual: %lu index: %lu db_index: %lu invalid: %lu\n",actual,index,PAY.buf[index].db_index,DEBUG_INDEX_INVALID);
    			PAY.buf[index].C_value.dw = actual->a_rdst[0].value;
        }
        break;
  #else
    // If checker is disabled, perform the operation
    case OP_MADD:
      if(inst.fmt() == FMT_S){
  			PAY.buf[index].C_value.f[1] = (uint32_t)0;
  			PAY.buf[index].C_value.f[0] = (PAY.buf[index].A_value.f[0]*PAY.buf[index].B_value.f[0]) + PAY.buf[index].D_value.f[0];
      } else if(inst.fmt() == FMT_D){
  			PAY.buf[index].C_value.d = (PAY.buf[index].A_value.d*PAY.buf[index].B_value.d) + PAY.buf[index].D_value.d;
      } else {
        assert(0);
      }
	  	break;
    case OP_MSUB:
      if(inst.fmt() == FMT_S){
  			PAY.buf[index].C_value.f[1] = (uint32_t)0;
  			PAY.buf[index].C_value.f[0] = (PAY.buf[index].A_value.f[0]*PAY.buf[index].B_value.f[0]) - PAY.buf[index].D_value.f[0];
      } else if(inst.fmt() == FMT_D){
  			PAY.buf[index].C_value.d = (PAY.buf[index].A_value.d*PAY.buf[index].B_value.d) - PAY.buf[index].D_value.d;
      } else {
        assert(0);
      }
	  	break;
    case OP_NMADD:
      if(inst.fmt() == FMT_S){
  			PAY.buf[index].C_value.f[1] = (uint32_t)0;
  			PAY.buf[index].C_value.f[0] = -((PAY.buf[index].A_value.f[0]*PAY.buf[index].B_value.f[0]) + PAY.buf[index].D_value.f[0]);
      } else if(inst.fmt() == FMT_D){
  			PAY.buf[index].C_value.d = -((PAY.buf[index].A_value.d*PAY.buf[index].B_value.d) + PAY.buf[index].D_value.d);
      } else {
        assert(0);
      }
	  	break;
    case OP_NMSUB:
      if(inst.fmt() == FMT_S){
  			PAY.buf[index].C_value.f[1] = (uint32_t)0;
  			PAY.buf[index].C_value.f[0] = -((PAY.buf[index].A_value.f[0]*PAY.buf[index].B_value.f[0]) - PAY.buf[index].D_value.f[0]);
      } else if(inst.fmt() == FMT_D){
  			PAY.buf[index].C_value.d = -((PAY.buf[index].A_value.d*PAY.buf[index].B_value.d) - PAY.buf[index].D_value.d);
      } else {
        assert(0);
      }
	  	break;
  #endif

    //TODO: Add support for AMO ops
    //NOTE: Currently handled at retire. 
    //Simply mark as completed here.
    case OP_AMO:
      //reg_t read_amo_value;
      //if(inst.funct3() == FN3_AMO_W){

      //  reg_t read_amo_value  = mmu->load_int32(PAY.buf[index].A_value.dw);
      //  uint32_t write_amo_value;
      //  switch(inst.funct5()){
      //    case FN5_AMO_SWAP:
      //      write_amo_value = PAY.buf[index].B_value.dw;
      //      break;
      //    case FN5_AMO_ADD:
      //      write_amo_value = PAY.buf[index].B_value.dw + read_amo_value;
      //      break;
      //    case FN5_AMO_XOR:
      //      write_amo_value = PAY.buf[index].B_value.dw ^ read_amo_value;
      //      break;
      //    case FN5_AMO_AND:
      //      write_amo_value = PAY.buf[index].B_value.dw & read_amo_value;
      //      break;
      //    case FN5_AMO_OR:
      //      write_amo_value = PAY.buf[index].B_value.dw | read_amo_value;
      //      break;
      //    case FN5_AMO_MIN:
      //      write_amo_value = std::min(int32_t(PAY.buf[index].B_value.dw) , int32_t(read_amo_value));
      //      break;
      //    case FN5_AMO_MAX:
      //      write_amo_value = std::max(int32_t(PAY.buf[index].B_value.dw) , int32_t(read_amo_value));
      //      break;
      //    case FN5_AMO_MINU:
      //      write_amo_value = std::min(uint32_t(PAY.buf[index].B_value.dw) , uint32_t(read_amo_value));
      //      break;
      //    case FN5_AMO_MAXU:
      //      write_amo_value = std::max(uint32_t(PAY.buf[index].B_value.dw) , uint32_t(read_amo_value));
      //      break;
      //    default:
      //      assert(0);
      //  }
      //  mmu->store_uint32(PAY.buf[index].A_value.dw,write_amo_value);
      //  
      //} else if(inst.funct3() == FN3_AMO_D) {

      //  read_amo_value  = mmu->load_int64(PAY.buf[index].A_value.dw);
      //  reg_t write_amo_value;
      //  switch(inst.funct5()){
      //    case FN5_AMO_SWAP:
      //      write_amo_value = PAY.buf[index].B_value.dw;
      //      break;
      //    case FN5_AMO_ADD:
      //      write_amo_value = PAY.buf[index].B_value.dw + read_amo_value;
      //      break;
      //    case FN5_AMO_XOR:
      //      write_amo_value = PAY.buf[index].B_value.dw ^ read_amo_value;
      //      break;
      //    case FN5_AMO_AND:
      //      write_amo_value = PAY.buf[index].B_value.dw & read_amo_value;
      //      break;
      //    case FN5_AMO_OR:
      //      write_amo_value = PAY.buf[index].B_value.dw | read_amo_value;
      //      break;
      //    case FN5_AMO_MIN:
      //      write_amo_value = std::min(int64_t(PAY.buf[index].B_value.dw) , int64_t(read_amo_value));
      //      break;
      //    case FN5_AMO_MAX:
      //      write_amo_value = std::max(int64_t(PAY.buf[index].B_value.dw) , int64_t(read_amo_value));
      //      break;
      //    case FN5_AMO_MINU:
      //      write_amo_value = std::min(PAY.buf[index].B_value.dw , read_amo_value);
      //      break;
      //    case FN5_AMO_MAXU:
      //      write_amo_value = std::max(PAY.buf[index].B_value.dw , read_amo_value);
      //      break;
      //    default:
      //      assert(0);
      //  }
      //  mmu->store_uint64(PAY.buf[index].A_value.dw,write_amo_value);
      //} else {
      //  assert(0);
      //}
			//PAY.buf[index].C_value.dw = read_amo_value;
			break;
//
// Floating Point ALU operations
//
    case OP_OP_FP:

      // Single precision floating point
      if(inst.fmt() == FMT_S){
        // Set the upper word to 0 in case of single precision floats
		    PAY.buf[index].C_value.w[1] = (uint32_t)0;

        switch(inst.funct5()){
		      case FN5_FADD:
		      	PAY.buf[index].C_value.f[0] = PAY.buf[index].A_value.f[0] + PAY.buf[index].B_value.f[0];
		      	break;

		      case FN5_FSUB:
		      	PAY.buf[index].C_value.f[0] = PAY.buf[index].A_value.f[0] - PAY.buf[index].B_value.f[0];
		      	break;

		      case FN5_FMUL:
		      	PAY.buf[index].C_value.f[0] = PAY.buf[index].A_value.f[0] * PAY.buf[index].B_value.f[0];
		      	break;

		      case FN5_FDIV:
		      	PAY.buf[index].C_value.f[0] = PAY.buf[index].A_value.f[0] / PAY.buf[index].B_value.f[0];
		      	break;

		      case FN5_FMV_I2FP:
            // This is how it is implemented in Spike. If in single precision
            // mode, the upper 32 bit will not be used.
		      	PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw;
		      	break;

		      case FN5_FMV_FP2I:
            if(inst.funct3() == FN3_FMV){
  		      	PAY.buf[index].C_value.dw = sext32(PAY.buf[index].A_value.w[0]);
            } else if(inst.funct3() == FN3_FCLASS){
              //TODO: Add classify operation
            } else {
              assert(0);
            }
		      	break;

          case FN5_FCVT_I2FP:
            switch(inst.rs2()){
		          case RS2_FCVT_W:
		          	PAY.buf[index].C_value.f[0] = (float)PAY.buf[index].A_value.sw[0];
		          	break;

		          case RS2_FCVT_WU:
		          	PAY.buf[index].C_value.f[0] = (float)PAY.buf[index].A_value.w[0];
		          	break;

		          case RS2_FCVT_L:
		          	PAY.buf[index].C_value.f[0] = (float)PAY.buf[index].A_value.sdw;
		          	break;

		          case RS2_FCVT_LU:
		          	PAY.buf[index].C_value.f[0] = (float)PAY.buf[index].A_value.dw;
		          	break;
            }
            break;

          case FN5_FCVT_FP2I:
            switch(inst.rs2()){
		          case RS2_FCVT_W:
		          	PAY.buf[index].C_value.sdw    = sext32((sword_t)PAY.buf[index].A_value.f[0]);
		          	break;

		          case RS2_FCVT_WU:
		          	PAY.buf[index].C_value.dw     = (reg_t)PAY.buf[index].A_value.f[0];
		          	break;

		          case RS2_FCVT_L:
		          	PAY.buf[index].C_value.sdw    = (sreg_t)PAY.buf[index].A_value.f[0];
		          	break;

		          case RS2_FCVT_LU:
		          	PAY.buf[index].C_value.dw     = (reg_t)PAY.buf[index].A_value.f[0];
		          	break;

            }
            break;

         // FCVT.S.D
         case FN5_FCVT_DS:
		        PAY.buf[index].C_value.f[0] = (float)PAY.buf[index].A_value.d;
		        break;

          case FN5_FCOMP:
            switch(inst.funct3()){
              case FN3_FEQ:
		          	PAY.buf[index].C_value.w[0] = (PAY.buf[index].A_value.f[0] == PAY.buf[index].B_value.f[0]);
		          	break;

              case FN3_FLT:
		          	PAY.buf[index].C_value.w[0] = (PAY.buf[index].A_value.f[0] < PAY.buf[index].B_value.f[0]);
		          	break;

              case FN3_FLE:
		          	PAY.buf[index].C_value.w[0] = (PAY.buf[index].A_value.f[0] <= PAY.buf[index].B_value.f[0]);
		          	break;
            }
            break;

		      case FN5_FSQRT:
		      	PAY.buf[index].C_value.f[0] = sqrt((double)PAY.buf[index].A_value.f[0]);
		      	break;

          case FN5_FSGNJ:
            switch(inst.funct3()){
              case FN3_FSGNJ:
                PAY.buf[index].C_value.w[0] = ( (PAY.buf[index].A_value.w[0] &~ (uint32_t)INT32_MIN) 
                                               |(PAY.buf[index].B_value.w[0] & (uint32_t)INT32_MIN));
                break;
              case FN3_FSGNJN:
                PAY.buf[index].C_value.w[0] = ( (PAY.buf[index].A_value.w[0] &~ (uint32_t)INT32_MIN) 
                                               |((~PAY.buf[index].B_value.w[0]) & (uint32_t)INT32_MIN));
                break;
              case FN3_FSGNJX:
                PAY.buf[index].C_value.w[0] = ( PAY.buf[index].A_value.w[0] 
                                               ^(PAY.buf[index].B_value.w[0] & (uint32_t)INT32_MIN));
                break;
            }
            break;

		      default:
		      	assert(0);
		      	break;

        }
      }

      // Double precision floating point
      else if(inst.fmt() == FMT_D){

        switch(inst.funct5()){
		      case FN5_FADD:
		      	PAY.buf[index].C_value.d = PAY.buf[index].A_value.d + PAY.buf[index].B_value.d;
		      	break;

		      case FN5_FSUB:
		      	PAY.buf[index].C_value.d = PAY.buf[index].A_value.d - PAY.buf[index].B_value.d;
		      	break;

		      case FN5_FMUL:
		      	PAY.buf[index].C_value.d = PAY.buf[index].A_value.d * PAY.buf[index].B_value.d;
		      	break;

		      case FN5_FDIV:
		      	PAY.buf[index].C_value.d = PAY.buf[index].A_value.d / PAY.buf[index].B_value.d;
		      	break;

		      case FN5_FMV_I2FP:
		      	PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw;
		      	break;

		      case FN5_FMV_FP2I:
            if(inst.funct3() == FN3_FMV){
  		      	PAY.buf[index].C_value.dw = PAY.buf[index].A_value.dw;
            } else if(inst.funct3() == FN3_FCLASS){
              //TODO: Add a classify operation
            } else {
              assert(0);
            }
		      	break;

          case FN5_FCVT_I2FP:
            switch(inst.rs2()){
		          case RS2_FCVT_W:
		          	PAY.buf[index].C_value.d = (double)PAY.buf[index].A_value.sw[0];
		          	break;

		          case RS2_FCVT_WU:
		          	PAY.buf[index].C_value.d = (double)PAY.buf[index].A_value.w[0];
		          	break;

		          case RS2_FCVT_L:
		          	PAY.buf[index].C_value.d = (double)PAY.buf[index].A_value.sdw;
		          	break;

		          case RS2_FCVT_LU:
		          	PAY.buf[index].C_value.d = (double)PAY.buf[index].A_value.dw;
		          	break;
            }
            break;

          case FN5_FCVT_FP2I:
            switch(inst.rs2()){
		          case RS2_FCVT_W:
		          	PAY.buf[index].C_value.dw  = sext32(PAY.buf[index].A_value.d);
		          	break;

		          case RS2_FCVT_WU:
		          	PAY.buf[index].C_value.dw  = zext32(PAY.buf[index].A_value.d);
		          	break;

		          case RS2_FCVT_L:
		          	PAY.buf[index].C_value.sdw = (sreg_t)PAY.buf[index].A_value.d;
		          	break;

		          case RS2_FCVT_LU:
		          	PAY.buf[index].C_value.dw  = (reg_t)PAY.buf[index].A_value.d;
		          	break;
            }
            break;

          //FCVT.D.S
          case FN5_FCVT_DS:
		        PAY.buf[index].C_value.d = (double)PAY.buf[index].A_value.f[0];
		        break;

          case FN5_FCOMP:
            switch(inst.funct3()){
		          case FN3_FEQ:
		          	PAY.buf[index].C_value.dw = (PAY.buf[index].A_value.d == PAY.buf[index].B_value.d);
		          	break;

		          case FN3_FLT:
		          	PAY.buf[index].C_value.dw = (PAY.buf[index].A_value.d < PAY.buf[index].B_value.d);
		          	break;

		          case FN3_FLE:
		          	PAY.buf[index].C_value.dw = (PAY.buf[index].A_value.d <= PAY.buf[index].B_value.d);
		          	break;
            }
            break;

		      case FN5_FSQRT:
		      	PAY.buf[index].C_value.d = sqrt(PAY.buf[index].A_value.d);
		      	break;

          case FN5_FSGNJ:
            switch(inst.funct3()){
              case FN3_FSGNJ:
                PAY.buf[index].C_value.dw = ( (PAY.buf[index].A_value.dw &~ INT64_MIN) 
                                               |(PAY.buf[index].B_value.dw & INT64_MIN));
                break;
              case FN3_FSGNJN:
                PAY.buf[index].C_value.dw = ( (PAY.buf[index].A_value.dw &~ INT64_MIN) 
                                               |((~PAY.buf[index].B_value.dw) & INT64_MIN));
                break;
              case FN3_FSGNJX:
                PAY.buf[index].C_value.dw = ( PAY.buf[index].A_value.dw 
                                               ^(PAY.buf[index].B_value.dw & INT64_MIN));
                break;
            }
            break;

		      default:
            assert(0);
		      	break;
        }
      }
      else{
        assert(0);
      }

      break; //OP_FP

	} //switch inst.opcode()

}



//////////////////////////////////////////////////
// non-expression instruction implementations
//////////////////////////////////////////////////

//
// rd <- ([rt] >> shamt)
//
static void ExecSRA(reg_t rt, unsigned int shamt, reg_t* rd)
{
	unsigned int i;
	reg_t temp;

	/* Although SRA could be implemented with the >> operator in the
	   MIPS machine, there are other machines that perform a logical
	   right shift with the >> operator. */
	if (rt & 020000000000) {
		temp = rt;
		for (i = 0; i < shamt; i++) {
			temp = (temp >> 1) | 020000000000 ;
		}

		*rd = temp;
	}
	else {
		*rd = (rt >> shamt);
	}
}

//
// rd <- [rt] >> [5 LSBs of rs])
//
static void
ExecSRAV(reg_t rt, reg_t rs, reg_t* rd)
{
	unsigned int i;
	reg_t temp;
	unsigned int shamt = (rs & 037);

	if (rt & 020000000000) {
		temp = rt;
		for (i = 0; i < shamt; i++) {
			temp = (temp >> 1) | 020000000000 ;
		}

		*rd = temp;
	}
	else {
		*rd = (rt >> shamt);
	}
}

//
// HI,LO <- [rs] * [rt]    Integer product of [rs] & [rt] to HI/LO
//
// op1 = RS
// op2 = RT
//
static void ExecMULT(reg_t op1, reg_t op2,
         reg_t* hi, reg_t* lo)
{
	int sign1, sign2;
	reg_t temp_hi, temp_lo;
	long i;

	sign1 = sign2 = 0;
	temp_hi = 0;
	temp_lo = 0;

	/* For multiplication, treat -ve numbers as +ve numbers by
	   converting 2's complement -ve numbers to ordinary notation */
	if (op1 & 020000000000) {
		sign1 = 1;
		op1 = (~op1) + 1;
	}
	if (op2 & 020000000000) {
		sign2 = 1;
		op2 = (~op2) + 1;
	}
	if (op1 & 020000000000) {
		temp_lo = op2;
	}
	for (i = 0; i < 31; i++) {
		temp_hi = (temp_hi << 1);
    //TODO: Fix this
		//temp_hi = (temp_hi + extractl(temp_lo, 31, 1));
		//temp_lo = (temp_lo << 1);
		//if ((extractl(op1, 30-i, 1)) == 1) {
		//	if (((unsigned)037777777777 - (unsigned)temp_lo) < (unsigned)op2) {
		//		temp_hi = (temp_hi + 1);
		//	}
		//	temp_lo = (temp_lo + op2);
		//}
	}

	/* Take 2's complement of the result if the result is negative */
	if (sign1 ^ sign2) {
		temp_lo = (~temp_lo);
		temp_hi = (~temp_hi);
		if ((unsigned)temp_lo == 037777777777) {
			temp_hi = (temp_hi + 1);
		}
		temp_lo = (temp_lo + 1);
	}

	*hi = temp_hi;
	*lo = temp_lo;
}

//
// HI,LO <- [rs] * [rt]    Integer product of [rs] & [rt] to HI/LO
//
static void ExecMULTU(reg_t rs, reg_t rt,
          reg_t* hi, reg_t* lo)
{
	long i;
	reg_t temp_hi, temp_lo;

	temp_hi = 0;
	temp_lo = 0;
	if (rs & 020000000000) {
		temp_lo = rt;
	}
	for (i = 0; i < 31; i++) {
		temp_hi = (temp_hi << 1);
    //TODO: Fix this
		//temp_hi = (temp_hi + extractl(temp_lo, 31, 1));
		//temp_lo = (temp_lo << 1);
		//if ((extractl(rs, 30-i, 1)) == 1) {
		//	if (((unsigned)037777777777 - (unsigned)temp_lo) < (unsigned)rt) {
		//		temp_hi = (temp_hi + 1);
		//	}
		//	temp_lo = (temp_lo + rt);
		//}
	}

	*hi = temp_hi;
	*lo = temp_lo;
}

