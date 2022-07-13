include ../../../Makefile.main.common

NAME = main

SRC_C = $(NAME).c
IR_FILES += $(SRC_C:%.c=%.bc)
OBJ_FILES = $(SRC_C:%.c=%.o)
COV_FILES = $(SRC_C:%.c=%.cov.o)

# Generate $(NAME).bc for $(NAME).c
$(NAME).bc : $(NAME).c
	$(CC) $(CFLAGS) $+ -o $@

# Generate $(NAME).o for $(NAME).c
$(NAME).o : $(NAME).c
	$(CC) -g -c $+
