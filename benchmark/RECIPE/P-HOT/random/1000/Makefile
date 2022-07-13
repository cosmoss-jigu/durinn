NAME = main

PM_FILE_PATH = pm.img
PM_SIZE = 8
PM_LAYOUT = layout
OP_FILE_PATH = op_file.txt
OUTPUT_PATH = output
INPUT = $(PM_FILE_PATH) $(PM_SIZE) $(PM_LAYOUT) $(OP_FILE_PATH) $(OUTPUT_PATH) 0

PM_ADDR = $(PMEM_MMAP_HINT)

CHECKER_OUT_PATH = checker-output

TRACE_EXE = $(DURINN_HOME)/benchmark/RECIPE/P-HOT/main/$(NAME).trace.exe
EXE = $(DURINN_HOME)/benchmark/RECIPE/P-HOT/main/$(NAME).exe
ALL_BC = $(DURINN_HOME)/benchmark/RECIPE/P-HOT/main/$(NAME).all.bc

NEED_DEL = false

include ../../../../Makefile.test.common
