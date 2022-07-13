CC = $(LLVM9_BIN)/clang
CXX = $(LLVM9_BIN)/clang++
INC_DIR = ../include
CFLAGS = -g -O0 -c -emit-llvm -I$(INC_DIR) -Wno-deprecated

DIR = ../../../third_party/FAST_FAIR/concurrent/src
FILES = $(DIR)/pmdk.c

all: bc obj

bc : pmdk.bc

pmdk.bc : copy
	$(CXX) $(CFLAGS) pmdk.c -o pmdk.bc

obj : copy
	$(CXX) -g -c -I$(INC_DIR) *.c

copy:
	@ cp $(FILES) .

clean:
	@ rm -f *.c *.bc *.o
