#include <cstdio>
#include <cassert>
#include <cinttypes>
#include "parameters.h"
#include "bpred_interface.h"
#include "stats.h"

//unsigned int HIST_MASK = 0xfffc;
//unsigned int HIST_BIT  = 0x8000;
//unsigned int PC_MASK   = 0x3fff;

//
// Constructor
//
// Initialize control flow prediction stuff.
//
bpred_interface::bpred_interface()
{
	unsigned int i;

	BTB = new BpredPredictAutomaton[BTB_SIZE];
	pred_table = new BpredPredictAutomaton[BP_TABLE_SIZE];
	conf_table = new BpredPredictAutomaton[BP_TABLE_SIZE];
	fm_table = new BpredPredictAutomaton[BP_TABLE_SIZE];		// "FM"
  cti_Q = new CTI_entry_b[CTIQ_SIZE];

	cti_head = 0;
	cti_tail = 0;
	for (i = 0; i < CTIQ_SIZE; i++)
	{
		cti_Q[i].RAS_action = 0;
		cti_Q[i].history = 0;
		cti_Q[i].state = 0;
	}

	for (i = 0; i < BTB_SIZE; i++)
	{
		BTB[i].tag = 0;
		BTB[i].pred = 0;
		BTB[i].hyst = 0;
	}

	for (i = 0; i < BP_TABLE_SIZE; i++)
	{
		pred_table[i].tag = 0;
		pred_table[i].pred = 1;
		pred_table[i].hyst = 0;
		conf_table[i].pred = CONF_MAX;
		fm_table[i].pred = FM_MAX;				// "FM"
	}

	RAS = NULL;

	stat_num_pred = 0;
	stat_num_miss = 0;
	stat_num_cond_pred = 0;
	stat_num_cond_miss = 0;
	stat_num_flush_RAS = 0;
	stat_num_conf_corr = 0;
	stat_num_conf_ncorr = 0;
	stat_num_nconf_corr = 0;
	stat_num_nconf_ncorr = 0;
}

//
// Destructor
//
// Generic destructor
//
bpred_interface::~bpred_interface()
{
	if (RAS) {
		delete RAS;
	}
	RAS = NULL;
}


//-------------------------------------------------------------------
//
// Internal functions
//
//-------------------------------------------------------------------

void bpred_interface::update()
{
	uint32_t	current;
	uint32_t	new_tail;

	//
	// Update pointers
	//
	current = cti_tail;
	new_tail = (current + 1) & (CTIQ_MASK);
	cti_tail = new_tail;

	//
	// Update history;
	//
	if (cti_Q[current].is_cond)
	{
		if (cti_Q[current].taken) {
			cti_Q[new_tail].history = (cti_Q[current].history >> 1) | HIST_BIT;
		}
		else {
			cti_Q[new_tail].history = (cti_Q[current].history >> 1);
		}
	}
	else
	{
		cti_Q[new_tail].history = cti_Q[current].history;
	}

}

void bpred_interface::RAS_update()
{
	RAS_node_b* ptr;

	if (cti_Q[cti_head].flush_RAS)
	{
		if (RAS) {
			delete (RAS);
		}
		RAS = NULL;
		return;
	}

	if (cti_Q[cti_head].RAS_action == -1)
	{
		//
		// POP
		//
		assert (RAS);
		ptr = RAS;
		RAS = RAS->next;

		assert (ptr->addr == cti_Q[cti_head].RAS_address);

		ptr->next = NULL;
		delete ptr;
	}
	else if (cti_Q[cti_head].RAS_action == 1)
	{
		//
		// PUSH
		//
		ptr = new RAS_node_b();
		ptr->addr = cti_Q[cti_head].RAS_address;
		ptr->next = RAS;
		RAS = ptr;
    inc_counter(ras_write_count);
	}
}

bool bpred_interface::RAS_lookup(uint32_t* target)
{
	int count = 0;
	uint32_t index;
	uint32_t exit = (cti_head - 1) & CTIQ_MASK;

  inc_counter(ras_read_count);

	for (index = (cti_tail - 1) & CTIQ_MASK;
	        index != exit;
	        index = (index - 1) & CTIQ_MASK)
	{
		count += cti_Q[index].RAS_action;
		if (cti_Q[index].flush_RAS)
		{
			*target = 0;
			return (false);
		}
		if (count == 1)
		{
			*target = (cti_Q[index].RAS_address);
			return (true);
		}
	}

	RAS_node_b* ptr = RAS;
	for ( ; count < 0; count++)
	{
		if (ptr == NULL) {
			break;
		}
		ptr = ptr->next;
	}

	if (ptr)
	{
		*target = ptr->addr;
		return (true);
	}
	else
	{
		*target = 0;
		return (false);
	}

}

void bpred_interface::update_predictions(bool fm)		// "FM"
{
	uint32_t	conf_index;
	uint32_t	pred_index;
	uint32_t	btb_index;
	uint32_t	history;

	//
	// Update prediction
	//
	if (cti_Q[cti_head].is_cond)
	{
		pred_index = ((cti_Q[cti_head].history & HIST_MASK) ^
		              ((cti_Q[cti_head].pc / insn_size) & PC_MASK)) & BP_INDEX_MASK;
		pred_table[pred_index].update((uint32_t)cti_Q[cti_head].taken);

		//
		// update conf
		//
		history = cti_Q[cti_head].history;
		if (cti_Q[cti_head].original_pred != cti_Q[cti_head].pc + 8) {
			history = (history >> 1) | HIST_BIT;
		}
		else {
			history = (history >> 1);
		}

		conf_index = ((history & HIST_MASK) ^
		              ((cti_Q[cti_head].pc / insn_size) & PC_MASK)) & BP_INDEX_MASK;
		bool corr = (cti_Q[cti_head].target == cti_Q[cti_head].original_pred);

		conf_table[conf_index].conf_update((uint32_t)corr, CONF_RESET, CONF_MAX);

		//
		// "FM": update conf
		//
		fm_table[pred_index].conf_update((uint32_t)!fm, FM_RESET, FM_MAX);

    inc_counter(bp_write_count);

	}

  inc_counter(ctiq_read_count);

	//
	// update BTB
	//
	if (cti_Q[cti_head].use_BTB)
	{
		btb_index = (cti_Q[cti_head].pc / insn_size) & BTB_MASK;
		BTB[btb_index].update(cti_Q[cti_head].target);

    inc_counter(btb_write_count);
	}

}



void bpred_interface::make_predictions(unsigned int branch_history)
{
	uint32_t	conf_index;
	uint32_t	pred_index;
	uint32_t	btb_index;
	uint32_t	temp_target;
	uint32_t	history;

	//
	// Predict if cti is "taken"
	//
	if (cti_Q[cti_tail].is_cond)
	{
		if (branch_history == 0xFFFFFFFF)
		{
			history = cti_Q[cti_tail].history;
			pred_index = ((cti_Q[cti_tail].history & HIST_MASK) ^
			              ((cti_Q[cti_tail].pc / insn_size) & PC_MASK)) & BP_INDEX_MASK;
		}
		else
		{
			history = branch_history;
			pred_index = ((branch_history & HIST_MASK) ^
			              ((cti_Q[cti_tail].pc / insn_size) & PC_MASK)) & BP_INDEX_MASK;
			cti_Q[cti_tail].use_global_history = true;
			cti_Q[cti_tail].global_history = branch_history;
		}

		cti_Q[cti_tail].taken = pred_table[pred_index].pred;

		if (cti_Q[cti_tail].taken) {
			history = (history >> 1) | HIST_BIT;
		}
		else {
			history = (history >> 1);
		}

		conf_index = ((history & HIST_MASK) ^
		              ((cti_Q[cti_tail].pc / insn_size) & PC_MASK)) & BP_INDEX_MASK;
		cti_Q[cti_tail].conf = conf_table[conf_index].pred;

		cti_Q[cti_tail].fm = fm_table[pred_index].pred;		// "FM"
    inc_counter(ctiq_write_count);
	}
	else
	{
		cti_Q[cti_tail].taken = true;
		cti_Q[cti_tail].conf = 0xff;
		cti_Q[cti_tail].fm = 0xff;				// "FM"
	}

	//
	// Predict cti target
	//
	if (cti_Q[cti_tail].is_return)
	{
		//
		// Use RAS
		//
		if (RAS_lookup(&temp_target))
		{
			cti_Q[cti_tail].target = temp_target;
			cti_Q[cti_tail].RAS_action = -1;
			cti_Q[cti_tail].RAS_address = temp_target;
		}
		else
		{
			cti_Q[cti_tail].target = 0;
			cti_Q[cti_tail].conf = 0;
		}
	}
	else
	{
		if (cti_Q[cti_tail].taken)
		{
			//
			// use BTB
			//
			if (cti_Q[cti_tail].use_BTB)
			{
				btb_index = (cti_Q[cti_tail].pc / insn_size) & BTB_MASK;
				cti_Q[cti_tail].target = BTB[btb_index].pred;
			}
			else
			{
				cti_Q[cti_tail].target = cti_Q[cti_tail].comp_target;
			}
		}
		else
		{
			//
			// (pc + insn_size) prediction
			//
			cti_Q[cti_tail].target = cti_Q[cti_tail].pc + insn_size;
		}
	}

	//
	// RAS update for calls
	//
	if (cti_Q[cti_tail].is_call)
	{
		cti_Q[cti_tail].RAS_action = 1;
		cti_Q[cti_tail].RAS_address = cti_Q[cti_tail].pc + insn_size;
	}
}

void bpred_interface::decode()
{
	insn_t	inst;
	inst = cti_Q[cti_tail].inst;

	cti_Q[cti_tail].use_BTB = false;
	cti_Q[cti_tail].is_cond = false;
	cti_Q[cti_tail].is_call = false;
	cti_Q[cti_tail].is_return = false;

	switch (inst.opcode())
	{
		case OP_JAL:
      // If the next PC is saved in X1 - a function call according to the ABI
      if(inst.rd() == 1){
			  cti_Q[cti_tail].is_call = true;
      } 
      // otherwise a regular unconditional jump - Use computed address from immediate in this case
			break;

		case OP_JALR:
      // If the next PC is discarder (X0) and jump address is X1 - a return according to the ABI
      if((inst.rd() == 0) && (inst.rs1() == 1)){
	  		cti_Q[cti_tail].is_return = true;
      // If the next PC is saved in X1 - a function call according to the ABI
      } else if(inst.rd() == 1){
  			cti_Q[cti_tail].use_BTB = true;
	  		cti_Q[cti_tail].is_call = true;
      // A regular unconditional jump with register as base address - Use BTB in this case
      } else {
  			cti_Q[cti_tail].use_BTB = true;
      }
			break;

		case OP_BRANCH :
			cti_Q[cti_tail].is_cond = true;
			break;

		default:
			break;
	}
}

//-------------------------------------------------------------------
//
// Interface functions
//
//-------------------------------------------------------------------

//
// 	get_pred()
//
// Get the next prediction.
// Returns the predicted target, and also assigns a tag to the prediction,
// which is used as a kind of sequence number by 'update_pred'.
//
unsigned int bpred_interface::get_pred(unsigned int branch_history,
                                       unsigned int PC, insn_t inst,
                                       unsigned int comp_target, unsigned int* pred_tag)
{

	unsigned int tag;

	tag = cti_tail;

	cti_Q[cti_tail].RAS_action = 0;
	cti_Q[cti_tail].RAS_address = 0;
	cti_Q[cti_tail].flush_RAS = false;
	cti_Q[cti_tail].pc = PC;
	cti_Q[cti_tail].inst = inst;
	cti_Q[cti_tail].comp_target = comp_target;
	cti_Q[cti_tail].state = 1;
	cti_Q[cti_tail].is_conf = true;
	cti_Q[cti_tail].use_global_history = false;

	decode();
	make_predictions(branch_history);
	cti_Q[cti_tail].original_pred = cti_Q[cti_tail].target;
	update();

	*pred_tag = tag;
	return (cti_Q[tag].target);

}
unsigned int bpred_interface::get_pred(unsigned int branch_history,
                                       unsigned int PC, insn_t inst,
                                       unsigned int comp_target, unsigned int* pred_tag,
                                       bool* conf,
                                       bool* fm)				// "FM"
{

	unsigned int tag;

	tag = cti_tail;

	cti_Q[cti_tail].RAS_action = 0;
	cti_Q[cti_tail].RAS_address = 0;
	cti_Q[cti_tail].flush_RAS = false;
	cti_Q[cti_tail].pc = PC;
	cti_Q[cti_tail].inst = inst;
	cti_Q[cti_tail].comp_target = comp_target;
	cti_Q[cti_tail].state = 1;
	cti_Q[cti_tail].is_conf = true;
	cti_Q[cti_tail].use_global_history = false;

	decode();
	make_predictions(branch_history);

	cti_Q[cti_tail].original_pred = cti_Q[cti_tail].target;
	if (cti_Q[cti_tail].conf > CONF_THRESHOLD) {
		*conf = true;
	}
	else {
		*conf = false;
	}

	// "FM"
	if (cti_Q[cti_tail].fm > FM_THRESHOLD) {
		*fm = false;
	}
	else {
		*fm = true;
	}

	update();

	*pred_tag = tag;
	return (cti_Q[tag].target);

}

void bpred_interface::set_nconf(unsigned int pred_tag)
{
	cti_Q[pred_tag].is_conf = false;
}

//
//	    fix_pred()
//
// Tells the predictor that the instruciton has executed and was found
// to be mispredicted.  This backs up the history to the point after the
// misprediction.
//
// The prediction is identified via a tag, originally gotten from get_pred.
//
void bpred_interface::fix_pred(unsigned int pred_tag, unsigned int next_pc)
{
	uint32_t temp_target;

	cti_tail = pred_tag;

	if (cti_Q[cti_tail].is_cond)
	{
		bool taken;
		taken = (next_pc != (cti_Q[cti_tail].pc + insn_size));
		cti_Q[cti_tail].taken = taken;
	}

	cti_Q[cti_tail].target = next_pc;

	if (cti_Q[cti_tail].is_return)
	{
		if ((RAS_lookup(&temp_target)) && (temp_target == next_pc))
		{
			cti_Q[cti_tail].RAS_action = -1;
			cti_Q[cti_tail].RAS_address = next_pc;
			cti_Q[cti_tail].flush_RAS = false;
		}
		else
		{
			cti_Q[cti_tail].RAS_action = 0;
			cti_Q[cti_tail].flush_RAS = true;
		}
	}

	update();
}


//
//      verify_pred()
//
// Tells the predictor to go ahead and update state for a given prediction.
//
// The prediction is identified via a tag, originally gotten from get_pred.
//
void bpred_interface::verify_pred(unsigned int pred_tag, unsigned int next_pc,
                                  bool fm)			// "FM"
{
	assert (pred_tag == cti_head);
	assert (cti_Q[cti_head].state == 1);
	//assert (next_pc = cti_Q[cti_head].target);
	assert (next_pc == cti_Q[cti_head].target);
	assert (!cti_Q[cti_head].use_global_history ||
	        (cti_Q[cti_head].global_history == cti_Q[cti_head].history));

	//
	// Update things
	//
	update_predictions(fm);					// "FM"
	RAS_update();

	//
	// STATS
	//
	stat_num_pred++;
	bool conf = cti_Q[cti_head].conf > CONF_THRESHOLD;
	//bool conf = cti_Q[cti_head].is_conf;
	if (cti_Q[cti_head].original_pred != next_pc)
	{
		stat_num_miss++;
		if (conf)
		{
			//printf("%08x (%08x) pred:%08x act:%08x\n", cti_Q[cti_head].pc,
			//    cti_Q[(cti_head - 1) & CTIQ_MASK].target,
			//	cti_Q[cti_head].original_pred,
			//	cti_Q[cti_head].target);
			stat_num_conf_ncorr++;
		}
		else {
			stat_num_nconf_ncorr++;
		}
	}
	else
	{
		if (conf) {
			stat_num_conf_corr++;
		}
		else {
			stat_num_nconf_corr++;
		}
	}
	if (cti_Q[cti_head].flush_RAS) {
		stat_num_flush_RAS++;
	}
	if (cti_Q[cti_head].is_cond)
	{
		stat_num_cond_pred++;
		if (cti_Q[cti_head].original_pred != next_pc) {
			stat_num_cond_miss++;
		}
	}

	//
	// Clear state
	//
	cti_Q[cti_head].RAS_action = 0;
	cti_Q[cti_head].RAS_address = 0;
	cti_Q[cti_head].flush_RAS = false;
	cti_Q[cti_head].pc = 0;
	cti_Q[cti_head].comp_target = 0;
	cti_Q[cti_head].state = 0;

	cti_head = (cti_head + 1) & CTIQ_MASK;
}

//
//	flush pending predictions
//
void bpred_interface::flush()
{
	cti_tail = cti_head;
}

//
// Dump branch predictor configuration.
//
void bpred_interface::dump_config(FILE* fp) {
	fprintf(fp, "BTB:\n");
	fprintf(fp, "   # entries    = %d\n", BTB_SIZE);
	fprintf(fp, "   pc mask      = 0x%x\n", BTB_MASK);
	fprintf(fp, "Cond. BP:\n");
	fprintf(fp, "   # entries    = %d\n", BP_TABLE_SIZE);
	fprintf(fp, "   index mask   = 0x%x\n", BP_INDEX_MASK);
	fprintf(fp, "   pc mask      = 0x%x\n", PC_MASK);
	fprintf(fp, "   history mask = 0x%x\n", HIST_MASK);
	fprintf(fp, "   history bit  = 0x%x\n", HIST_BIT);
}

//
// Dump stats
//
void bpred_interface::dump_stats(FILE* fp) {
	fprintf(fp, "all branches\n");
	fprintf(fp, "   predictions:         %d\n", stat_num_pred);
	fprintf(fp, "   mispredictions:      %d\n", stat_num_miss);
	fprintf(fp, "   misprediction ratio: %f\n", ((float)stat_num_miss/(float)stat_num_pred));
	fprintf(fp, "conditional branches only\n");
	fprintf(fp, "   predictions:         %d\n", stat_num_cond_pred);
	fprintf(fp, "   mispredictions:      %d\n", stat_num_cond_miss);
	fprintf(fp, "   misprediction ratio: %f\n", ((float)stat_num_cond_miss/(float)stat_num_cond_pred));
}

unsigned int bpred_interface::update_global_history(unsigned int ghistory,
        insn_t inst, unsigned int pc,
        unsigned int next_pc)
{
	switch (inst.opcode())
	{
		case OP_BRANCH :
			if (next_pc != INCREMENT_PC(pc)) { // if taken
				ghistory = (ghistory >> 1) | HIST_BIT;
			}
			else {
				ghistory = (ghistory >> 1);
			}
			return (ghistory);
			break;
		default:
			return (ghistory);
			break;
	}
}
