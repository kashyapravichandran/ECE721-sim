
TOP = ..

all:
	make $(riscv_gen_srcs);

clean:
	rm -f *.cc *.o
	
	
#
#sed 's/NAME/fmv_s_x/' ../riscv/insn_template.cc | sed 's/OPCODE/ MATCH_FMV_S_X/' > fmv_s_x.cc
#

get_insn_list = $(shell grep ^DECLARE_INSN $(1) | sed 's/DECLARE_INSN(\(.*\),.*,.*)/\1/')
get_opcode = $(shell grep ^DECLARE_INSN.*\\\<$(2)\\\> $(1) | sed 's/DECLARE_INSN(.*,\(.*\),.*)/\1/')

riscv_gen_srcs = \
	$(addsuffix .cc, $(call get_insn_list,$(TOP)/encoding.h))

$(riscv_gen_srcs): %.cc: %.h $(TOP)/insn_template.cc
	sed 's/NAME/$(subst .cc,,$@)/' $(TOP)/insn_template.cc | sed 's/OPCODE/$(call get_opcode,$(TOP)/encoding.h,$(subst .cc,,$@))/' > $@

