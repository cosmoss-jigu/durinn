NAME = main

PM_FILE_PATH = pm.img
PM_SIZE = 8
PM_LAYOUT = layout
OP_FILE_PATH = op_file.txt
OUTPUT_PATH = output
INPUT = $(PM_FILE_PATH) $(PM_SIZE) $(PM_LAYOUT) $(OP_FILE_PATH) $(OUTPUT_PATH) 0

PM_ADDR = $(PMEM_MMAP_HINT)

CHECKER_OUT_PATH = checker-output

TRACE_EXE = $(DURINN_HOME)/benchmark/NVTraverse/Skiplist/main/$(NAME).trace.exe
EXE = $(DURINN_HOME)/benchmark/NVTraverse/Skiplist/main/$(NAME).exe
ALL_BC = $(DURINN_HOME)/benchmark/NVTraverse/Skiplist/main/$(NAME).all.bc

LOCK_FREE = 1

include ../../../../Makefile.test.common
