INCDIR = include
MAINDIR = main

.PHONY: all main include clean

all : main

main: include
	$(MAKE) -C $(MAINDIR)

include:
	$(MAKE) -C $(INCDIR)

clean:
	$(MAKE) -C $(INCDIR) clean
	$(MAKE) -C $(MAINDIR) clean
