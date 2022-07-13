NAME = check

LIB_DIR = ../lib
SRC_C = $(NAME).c $(LIB_DIR)/*.c
IR_FILES = $(SRC_C:%.c=%.bc)
OBJ_FILES = $(SRC_C:%.c=%.o)

#GIRI_DIR = ../../../giri/build
GIRI_DIR = ../../../../giri/build-llvm9

INC_DIR0 = ../include
INC_DIR1 = $(GIRI_DIR)/../include/Witcher

EXT_TRACING_FUNC_FILE = ../../../giri/ext.tracing.func.txt

all: $(NAME).exe

# Generate a pure executable
$(NAME).exe : $(NAME).o
	$(CC) -g $(OBJ_FILES) -lm -lpmemobj -lpmdktrace -o $@
# Generate $(NAME).o for $(NAME).c

$(NAME).o : $(NAME).c
	$(CC) -g -O0 -c -I$(INC_DIR0) -I$(INC_DIR1) -DCLWB $+

clean:
	rm -f $(NAME).o $(NAME).exe
