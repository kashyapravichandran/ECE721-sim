CC = g++
#INC = -I. -I../src1_me/riscv-fesvr -I../src1_me/riscv-isa-sim/softfloat -I../src1_me/install/include/spike
INC = -I. -I./softfloat -I./spike_main -I../uarchsim
#LIBS = -lpthread -lz -ldl
OPT = -O3
FLAGS = -std=c++11 -DPREFIX=\".\" -DRISCV_MICRO_CHECKER $(INC) $(LIBS) $(OPT)

TOP = .

OBJ_SPIKE = $(patsubst %.cc,%.o,$(wildcard ./spike_main/*.cc))
OBJ_INSN = $(patsubst %.cc,%.o,$(wildcard ./insns/*.cc))
OBJ_FESVR = $(patsubst %.cc,%.o,$(wildcard ./fesvr/*.cc))
OBJ_SOFTFLOAT = $(patsubst %.c,%.o,$(wildcard ./softfloat/*.c))
OBJ = bbtracker.o  cachesim.o  extension.o  gzstream.o  htif.o  interactive.o  mmu.o  processor.o  regnames.o  rocc.o  trap.o

all: icache.h libriscv-base.a

libriscv-base.a: $(OBJ) $(OBJ_INSN) $(OBJ_FESVR) $(OBJ_SOFTFLOAT) $(OBJ_SPIKE)
	ar r $@ $^

#insns_template.o:
#	echo "*** not compiling insns_template.cc (as instructed) ***"

%.o: %.cc $(DEPS)
	$(CC) $(FLAGS) -c -o $@ $<

%.o: %.c $(DEPS)
	$(CC) $(FLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -f icache.h *.o spike_main/*.o insns/*.o fesvr/*.o softfloat/*.o libriscv-base.a

#-------------------------------------------------------------------------------------------------------

#sed 's/NAME/fmv_s_x/' ../riscv/insn_template.cc | sed 's/OPCODE/ MATCH_FMV_S_X/' > fmv_s_x.cc


get_insn_list = $(shell grep ^DECLARE_INSN $(1) | sed 's/DECLARE_INSN(\(.*\),.*,.*)/\1/')
get_opcode = $(shell grep ^DECLARE_INSN.*\\\<$(2)\\\> $(1) | sed 's/DECLARE_INSN(.*,\(.*\),.*)/\1/')

riscv_subproject_deps = \
	softfloat \

riscv_install_prog_srcs = \

riscv_hdrs = \
	htif.h \
	common.h \
	decode.h \
	mmu.h \
	processor.h \
	debug.h \
	bp.h \
	uarchsim.h \
	parameters.h \
	fifo.h \
	sim.h \
	trap.h \
	encoding.h \
	cachesim.h \
	memtracer.h \
	extension.h \
	rocc.h \
	insn_template.h \
	mulhi.h \
	bbtracker.h	\
	gzstream.h	\

riscv_precompiled_hdrs = \
	insn_template.h \

riscv_srcs = \
	htif.cc \
	processor.cc \
	debug.cc \
	bp.cc \
	uarchsim.cc \
	parameters.cc \
	sim.cc \
	interactive.cc \
	trap.cc \
	cachesim.cc \
	mmu.cc \
	disasm.cc \
	extension.cc \
	rocc.cc \
	regnames.cc \
	bbtracker.cc	\
	gzstream.cc	\
	$(riscv_gen_srcs) \

riscv_test_srcs =

riscv_gen_hdrs = \
  icache.h \

riscv_gen_srcs = \
	$(addsuffix .cc, $(call get_insn_list,$(TOP)/encoding.h))

icache_entries := `grep "ICACHE_ENTRIES =" $(TOP)/mmu.h | sed 's/.* = \(.*\);/\1/'`

icache.h: mmu.h
	$(TOP)/gen_icache $(icache_entries) > $@.tmp
	mv $@.tmp $@

$(riscv_gen_srcs): %.cc: $(TOP)/insns/%.h $(TOP)/insns/insn_template.cc
	sed 's/NAME/$(subst .cc,,$@)/' $(TOP)/insns/insn_template.cc | sed 's/OPCODE/$(call get_opcode,$(TOP)/encoding.h,$(subst .cc,,$@))/' > $@

riscv_junk = \
	$(riscv_gen_srcs) \
