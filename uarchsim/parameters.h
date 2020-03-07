#ifndef PARAMETERS_H
#define PARAMETERS_H
#include <cinttypes>

// Pipe control
extern unsigned int PIPE_QUEUE_SIZE;


// Oracle controls.
extern bool PERFECT_BRANCH_PRED;
extern bool PERFECT_FETCH;
extern bool ORACLE_DISAMBIG;
extern bool PERFECT_ICACHE;
extern bool PERFECT_DCACHE;

// Core.
extern unsigned int FETCH_QUEUE_SIZE;
extern unsigned int NUM_CHECKPOINTS;
extern unsigned int ACTIVE_LIST_SIZE;
extern unsigned int ISSUE_QUEUE_SIZE;
extern unsigned int ISSUE_QUEUE_NUM_PARTS;
extern unsigned int LQ_SIZE;
extern unsigned int SQ_SIZE;
extern unsigned int FETCH_WIDTH;
extern unsigned int DISPATCH_WIDTH;
extern unsigned int ISSUE_WIDTH;
extern unsigned int RETIRE_WIDTH;
extern bool         IC_INTERLEAVED;
extern bool         IC_SINGLE_BB;		// not used currently
extern bool         IN_ORDER_ISSUE;		// not used currently
extern bool         SPEC_DISAMBIG;
extern bool         MEM_DEP_PRED;
extern bool         PRESTEER;
extern bool         IDEAL_AGE_BASED;
extern unsigned int FU_LANE_MATRIX[];
extern unsigned int FU_LAT[];

// L1 Data Cache.
extern unsigned int L1_DC_SETS;
extern unsigned int L1_DC_ASSOC;
extern unsigned int L1_DC_LINE_SIZE;
extern unsigned int L1_DC_HIT_LATENCY;
extern unsigned int L1_DC_MISS_LATENCY;
extern unsigned int L1_DC_NUM_MHSRs;
extern unsigned int L1_DC_MISS_SRV_PORTS;
extern unsigned int L1_DC_MISS_SRV_LATENCY;

// L1 Instruction Cache.
extern unsigned int L1_IC_SETS;
extern unsigned int L1_IC_ASSOC;
extern unsigned int L1_IC_LINE_SIZE;
extern unsigned int L1_IC_HIT_LATENCY;
extern unsigned int L1_IC_MISS_LATENCY;
extern unsigned int L1_IC_NUM_MHSRs;
extern unsigned int L1_IC_MISS_SRV_PORTS;
extern unsigned int L1_IC_MISS_SRV_LATENCY;

// L2 Unified Cache.
extern bool         L2_PRESENT;
extern unsigned int L2_SETS;
extern unsigned int L2_ASSOC;
extern unsigned int L2_LINE_SIZE;  // 2^LINE_SIZE bytes per line
extern unsigned int L2_HIT_LATENCY;
extern unsigned int L2_MISS_LATENCY;
extern unsigned int L2_NUM_MHSRs; 
extern unsigned int L2_MISS_SRV_PORTS;
extern unsigned int L2_MISS_SRV_LATENCY;

// Branch predictor and BTB
extern unsigned int BTB_SIZE;
extern unsigned int BTB_MASK;
extern unsigned int BP_TABLE_SIZE;
extern unsigned int BP_INDEX_MASK;
extern unsigned int CTIQ_SIZE;
extern unsigned int CTIQ_MASK;
extern unsigned int RAS_SIZE;

// Branch predictor confidence.
extern bool CONF_RESET;
extern unsigned int CONF_THRESHOLD;
extern unsigned int CONF_MAX;

extern bool FM_RESET;
extern unsigned int FM_THRESHOLD;
extern unsigned int FM_MAX;

// Benchmark control.
extern bool logging_on;
extern int64_t logging_on_at;

extern bool use_stop_amt;
extern uint64_t stop_amt;

extern uint64_t phase_interval;
extern uint64_t verbose_phase_counters;

#endif //PARAMETERS_H
