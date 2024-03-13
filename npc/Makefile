BUILD_DIR = ./build
VERILATOR_FLAGS=--cc --exe --build --trace

export PATH := $(PATH):$(abspath ./utils)

test:
	mill -i __.test

verilog:
	$(call git_commit, "generate verilog")
	mkdir -p $(BUILD_DIR)
	mill -i __.test.runMain Elaborate -td $(BUILD_DIR)

help:
	mill -i __.test.runMain Elaborate --help

compile:
	mill -i __.compile

bsp:
	mill -i mill.bsp.BSP/install

reformat:
	mill -i __.reformat

checkformat:
	mill -i __.checkFormat

clean:
	-rm -rf $(BUILD_DIR)
	-rm -rf obj_dir

.PHONY: test verilog help compile bsp reformat checkformat clean

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	verilator $(VERILATOR_FLAGS) ./csrc/*.cpp ./vsrc/*.v

wave:
	./obj_dir/Vour
	gtkwave *.vcd
-include ../Makefile
