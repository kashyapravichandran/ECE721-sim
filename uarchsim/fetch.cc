#include "pipeline.h"
#include "mmu.h"
#include "CacheClass.h"


void pipeline_t::fetch() {
   // Variables related to instruction cache.
   unsigned int line1;
   unsigned int line2;
   bool hit1;
   bool hit2;
   cycle_t resolve_cycle1;
   cycle_t resolve_cycle2;

   // Variables influencing when to terminate fetch bundle.
   unsigned int i;	// iterate up to fetch width
   bool stop;		// branch, icache block boundary, etc.

   // Instruction fetched from mmu.
   insn_t insn;		// "insn" is used by some MACROs, hence, need to use this name for the instruction variable.
   reg_t trap_cause = 0;
 
   // PAY index for newly fetched instruction.
   unsigned int index;

   // Link to corresponding instrution in functional simulator, i.e., map_to_actual() functionality.
   db_t *actual;
 
   // Local variables related to branch prediction.
   unsigned int history_reg;
   unsigned int direct_target;
   reg_t next_pc;
   unsigned int pred_tag;
   bool conf;
   bool fm;


   /////////////////////////////
   // Stall logic.
   /////////////////////////////

   // Stall the Fetch Stage if either:
   // 1. The Decode Stage is stalled.
   // 2. An I$ miss has not yet resolved.
   if ((DECODE[0].valid) ||		// Decode Stage is stalled.
       (cycle < next_fetch_cycle)) {	// I$ miss has not yet resolved.
      return;
   }

   /////////////////////////////
   // Model I$ misses.
   /////////////////////////////

   if (!PERFECT_ICACHE) {
      line1 = (pc >> L1_IC_LINE_SIZE);
      resolve_cycle1 = IC->Access(Tid, cycle, (line1 << L1_IC_LINE_SIZE), false, &hit1);
      if (IC_INTERLEAVED) {
         // Access next consecutive line.
         line2 = (pc >> L1_IC_LINE_SIZE) + 1;
         resolve_cycle2 = IC->Access(Tid, cycle, (line2 << L1_IC_LINE_SIZE), false, &hit2);
      }
      else {
         hit2 = true;
      }

      if (!hit1 || !hit2) {
         next_fetch_cycle = MAX((hit1 ? 0 : resolve_cycle1), (hit2 ? 0 : resolve_cycle2));
         assert(next_fetch_cycle > cycle);
         return;
      }
   }

   /////////////////////////////
   // Compose fetch bundle.
   /////////////////////////////

   i = 0;
   stop = false;
   while ((i < fetch_width) && (PERFECT_FETCH || !stop)) {

      //////////////////////////////////////////////////////
      // Fetch instruction -or- inject NOP for fetch stall.
      //////////////////////////////////////////////////////

      if (fetch_exception || fetch_csr || fetch_amo) {
         // Stall the fetch unit if there is a prior unresolved fetch exception, CSR instruction, or AMO instruction.
         // A literal stall may deadlock the Rename Stage: it requires a full bundle from the FQ to progress.
         // Thus, instead of literally stalling the fetch unit, stall it by injecting NOPs after the offending
         // instruction until the offending instruction is resolved in the Retire Stage.
         insn = insn_t(INSN_NOP);
      }
      else {
         // Try fetching the instruction via the MMU.
         // Generate a "NOP with fetch exception" if the MMU reference generates an exception.
         try {
            insn = (mmu->load_insn(pc)).insn;
         }
         catch (trap_t& t) {
            insn = insn_t(INSN_NOP);
            set_fetch_exception();
            trap_cause = t.cause();
         }
      }

      if (insn.opcode() == OP_AMO)
         set_fetch_amo();
      else if (insn.opcode() == OP_SYSTEM)
         set_fetch_csr();
		
      // Put the instruction's information into PAY.
      index = PAY.push();
      PAY.buf[index].inst = insn;
      PAY.buf[index].pc = pc;
      PAY.buf[index].sequence = sequence;
      PAY.buf[index].fetch_exception = fetch_exception;
      PAY.buf[index].fetch_exception_cause = trap_cause;

      //////////////////////////////////////////////////////
      // map_to_actual()
      //////////////////////////////////////////////////////

      // Try to link the instruction to the corresponding instruction in the functional simulator.
      // NOTE: Even when NOPs are injected, successfully mapping to actual is not a problem,
      // as the NOP instructions will never be committed.
      PAY.map_to_actual(this, index, Tid);
      if (PAY.buf[index].good_instruction)
         actual = pipe->peek(PAY.buf[index].db_index);
      else
         actual = (db_t *) NULL;

      //////////////////////////////////////////////////////
      // Set next_pc and the prediction tag.
      //////////////////////////////////////////////////////

      // Initialize some predictor-related flags.
      pred_tag = 0;
      history_reg = 0xFFFFFFFF;

      switch (insn.opcode()) {
         case OP_JAL:
            direct_target = JUMP_TARGET;
            next_pc = (PERFECT_BRANCH_PRED ?
                          (actual ? actual->a_next_pc : direct_target) :
                          BP.get_pred(history_reg, pc, insn, direct_target, &pred_tag, &conf, &fm));
            assert(next_pc == direct_target);
            stop = true;
            break;

         case OP_JALR:
            next_pc = (PERFECT_BRANCH_PRED ?
                          (actual ? actual->a_next_pc : INCREMENT_PC(pc)) :
                          BP.get_pred(history_reg, pc, insn, 0, &pred_tag, &conf, &fm));
            stop = true;
            break;

         case OP_BRANCH:
            direct_target = BRANCH_TARGET;
            next_pc = (PERFECT_BRANCH_PRED ?
                          (actual ? actual->a_next_pc : INCREMENT_PC(pc)) :
                          BP.get_pred(history_reg, pc, insn, direct_target, &pred_tag, &conf, &fm));
            assert((next_pc == direct_target) || (next_pc == INCREMENT_PC(pc)));
            if (next_pc != INCREMENT_PC(pc))
               stop = true;
            break;

         default:
            next_pc = INCREMENT_PC(pc);
            break;
      }

      // Set payload buffer entry's next_pc and pred_tag.
      PAY.buf[index].next_pc = next_pc;
      PAY.buf[index].pred_tag = pred_tag;

      // Latch instruction into fetch-decode pipeline register.
      DECODE[i].valid = true;
      DECODE[i].index = index;

      // Keep count of number of fetched instructions.
      i++;

      // If not already stopped:
      // Stop if the I$ is not interleaved and if a line boundary is crossed.
      if (!stop && !IC_INTERLEAVED) {
         line1 = (pc >> L1_IC_LINE_SIZE);
         line2 = (next_pc >> L1_IC_LINE_SIZE);
         stop = (line1 != line2);
      }

      // Go to next PC.
      pc = next_pc;
      state.pc = pc;
      sequence++;
   }			// while()
}			// fetch()
