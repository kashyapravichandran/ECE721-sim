#include <cinttypes>
#include "fu.h"

// Pipe control
uint32_t PIPE_QUEUE_SIZE  = 4096;



// Oracle controls.
bool PERFECT_BRANCH_PRED	= false;
bool PERFECT_FETCH		    = false;
bool ORACLE_DISAMBIG		  = false;
bool PERFECT_ICACHE		    = false;
bool PERFECT_DCACHE		    = false;

// Core.
uint32_t FETCH_QUEUE_SIZE	= 32;
uint32_t NUM_CHECKPOINTS	= 32;
uint32_t ACTIVE_LIST_SIZE	= 256;
uint32_t ISSUE_QUEUE_SIZE	= 32;
uint32_t ISSUE_QUEUE_NUM_PARTS	= 4;
uint32_t LQ_SIZE		      = 32;
uint32_t SQ_SIZE		      = 32;
uint32_t FETCH_WIDTH	    = 8;//2;//4;
uint32_t DISPATCH_WIDTH	  = 8;//2;//4;
uint32_t ISSUE_WIDTH	    = 8;//3;//8;
uint32_t RETIRE_WIDTH	    = 8;//1;//4;
bool IC_INTERLEAVED		    = false;
bool IC_SINGLE_BB		      = false;	// not used currently
bool IN_ORDER_ISSUE		    = false;	// not used currently
bool SPEC_DISAMBIG = false;
bool MEM_DEP_PRED = false;

bool PRESTEER = false;
bool IDEAL_AGE_BASED = false;
uint32_t FU_LANE_MATRIX[(unsigned int)NUMBER_FU_TYPES] = {0x5A5A /*     BR: 0101 1010 */ ,
                                                          0x2121 /*     LS: 0010 0001 */ ,
                                                          0x5A5A /*  ALU_S: 0101 1010 */ ,
                                                          0x8484 /*  ALU_C: 1000 0100 */ ,
                                                          0x2121 /*  LS_FP: 0010 0001 */ ,
                                                          0x8484 /* ALU_FP: 1000 0100 */ ,
                                                          0x5A5A /*    MTF: 0101 1010 */
                                                         };
uint32_t FU_LAT[(unsigned int)NUMBER_FU_TYPES] = {1 /* BR     */ ,
                                                  3 /* LS     */ ,
                                                  1 /* ALU_S  */ ,
                                                  3 /* ALU_C  */ ,
                                                  3 /* LS_FP  */ ,
                                                  3 /* ALU_FP */ ,
                                                  1 /* MTF    */
                                                 };

//uint32_t FU_LANE_MATRIX[(unsigned int)NUMBER_FU_TYPES] = {0x04 /*     BR: 0000 0100 */ ,
//                                                          0x03 /*     LS: 0000 0011 */ ,
//                                                          0xf8 /*  ALU_S: 1111 1000 */ ,
//                                                          0x18 /*  ALU_C: 0001 1000 */ ,
//                                                          0x03 /*  LS_FP: 0000 0011 */ ,
//                                                          0x08 /* ALU_FP: 0000 1000 */ ,
//                                                          0x08 /*    MTF: 0000 1000 */
//                                                         };
//uint32_t FU_LANE_MATRIX[(unsigned int)NUMBER_FU_TYPES] = {0xff /*     BR: 0000 0100 */ ,
//                                                          0xff /*     LS: 0000 0011 */ ,
//                                                          0xff /*  ALU_S: 1111 1000 */ ,
//                                                          0xff /*  ALU_C: 0001 1000 */ ,
//                                                          0xff /*  LS_FP: 0000 0011 */ ,
//                                                          0xff /* ALU_FP: 0000 1000 */ ,
//                                                          0xff /*    MTF: 0000 1000 */
//                                                         };
                                                         

// L1 Data Cache.
unsigned int L1_DC_SETS             = 256;
unsigned int L1_DC_ASSOC            = 4;
unsigned int L1_DC_LINE_SIZE        = 6;  // 2^LINE_SIZE bytes per line
unsigned int L1_DC_HIT_LATENCY      = 1;
unsigned int L1_DC_MISS_LATENCY     = 100; // Used only when no L2 cache
unsigned int L1_DC_NUM_MHSRs        = 64; 
unsigned int L1_DC_MISS_SRV_PORTS   = 64;
unsigned int L1_DC_MISS_SRV_LATENCY = 1;

// L1 Instruction Cache.
unsigned int L1_IC_SETS             = 128;
unsigned int L1_IC_ASSOC            = 8;
unsigned int L1_IC_LINE_SIZE        = 6;	// 2^LINE_SIZE bytes per line
unsigned int L1_IC_HIT_LATENCY      = 1;
unsigned int L1_IC_MISS_LATENCY     = 100; // Used only when no L2 cache
unsigned int L1_IC_NUM_MHSRs        = 32;
unsigned int L1_IC_MISS_SRV_PORTS   = 1;
unsigned int L1_IC_MISS_SRV_LATENCY = 1;

// L2 Unified Cache.
bool         L2_PRESENT           = true;
unsigned int L2_SETS              = 512;
unsigned int L2_ASSOC             = 8;
unsigned int L2_LINE_SIZE         = 6;  // 2^LINE_SIZE bytes per line
unsigned int L2_HIT_LATENCY       = 10;
unsigned int L2_MISS_LATENCY      = 100;  // Used only when no L3
unsigned int L2_NUM_MHSRs         = 64; 
unsigned int L2_MISS_SRV_PORTS    = 64;
unsigned int L2_MISS_SRV_LATENCY  = 1;

// Size of Q for remembering outstanding predictions
unsigned int CTIQ_SIZE	            = 1024;
unsigned int CTIQ_MASK	            = CTIQ_SIZE-1;

// BTB configuration
unsigned int BTB_SIZE	              = 0x1000;
unsigned int BTB_MASK	              = BTB_SIZE-1;

// Predictor configuration
unsigned int BP_TABLE_SIZE	        = 0x10000;
unsigned int BP_INDEX_MASK	        = BP_TABLE_SIZE-1;

// RAS configuration
unsigned int RAS_SIZE = 32; 

// Branch predictor confidence.
bool CONF_RESET                     = true;
unsigned int CONF_THRESHOLD         = 14;
unsigned int CONF_MAX               = 15;

bool FM_RESET                       = true;
unsigned int FM_THRESHOLD           = 14;
unsigned int FM_MAX                 = 15;


// Benchmark control.
bool logging_on                     = false;
int64_t logging_on_at               = -2;  //0xfffffffffffffffe

bool use_stop_amt                   = false;
uint64_t stop_amt                   = 0xffffffffffffffff;

uint64_t phase_interval             = 10000;
uint64_t verbose_phase_counters     = true;
