CC = g++
RVBASE = ../riscv-base
INC = -I. -I$(RVBASE) -I$(RVBASE)/softfloat -I$(RVBASE)/spike_main
LIB = -L$(RVBASE)
OPT = -O3
#OPT = -g
FLAGS = -std=c++11 -DPREFIX=\".\" -DRISCV_MICRO_CHECKER $(INC) $(LIB) $(OPT)

OBJ = $(patsubst %.cc,%.o,$(wildcard ./*.cc))
LIBS = -lriscv-base -lpthread -lz -ldl 

all: 721sim

721sim: $(OBJ)
	$(CC) $(FLAGS) -o $@ $(OBJ) $(LIBS)
	@echo "-----------DONE WITH 721sim-----------"

%.o: %.cc $(DEPS)
	$(CC) $(FLAGS) -c -o $@ $<

clean:
	rm -f *.o 721sim
