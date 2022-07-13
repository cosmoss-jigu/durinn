CC = $(LLVM9_BIN)/clang
CXX = $(LLVM9_BIN)/clang++
INC_DIR = ../include
CFLAGS = -g -O0 -c -I$(INC_DIR) -Wno-deprecated -DCLWB

DIR = ../../../../third_party/RECIPE/P-CLHT/src
EXT_DIR = ../../../../third_party/RECIPE/P-CLHT/external/sspfd
EXT_DIR1 = ../../../../third_party/RECIPE/P-CLHT/external/ssmem/src

FILES = $(DIR)/pmdk.c $(DIR)/clht_lb_res.c

all: bc obj

bc : pmdk.bc clht_lb_res.bc

clht_lb_res.bc : copy
	$(CC) $(CFLAGS) -emit-llvm clht_lb_res.c
pmdk.bc : copy
	$(CC) $(CFLAGS) -emit-llvm  pmdk.c

obj : pmdk.o clht_lb_res.o

clht_lb_res.o : copy
	$(CC) $(CFLAGS)  clht_lb_res.c
pmdk.o : copy
	$(CC) $(CFLAGS) pmdk.c
copy:
	@ cp $(FILES) .

clean:
	@ rm -f *.cpp *.bc *.o *.c
