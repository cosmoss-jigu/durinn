include ../../../Makefile.main.common

NAME = main

LIB_DIR = ../lib
SRC_C = $(NAME).c $(LIB_DIR)/pmdk.c $(LIB_DIR)/clht_lb_res.c
IR_FILES = $(SRC_C:%.c=%.bc)
OBJ_FILES = $(SRC_C:%.c=%.o)

INC_DIR0 = ../include
CFLAGS += -I$(INC_DIR0) -Wno-deprecated -DCLWB

LINK_LIBS += -ltbb -latomic -lm -lpthread

# Generate $(NAME).bc for $(NAME).c
$(NAME).bc : $(NAME).c
	$(CC) $(CFLAGS) $+ -o $@

# Generate $(NAME).o for $(NAME).c
$(NAME).o : $(NAME).c
	$(CC) -g -O0 -c -I$(INC_DIR0) -DCLWB $+
