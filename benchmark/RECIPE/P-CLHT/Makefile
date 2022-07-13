INCDIR = include
LIBDIR = lib
MAINDIR = main
CHECKDIR = check

.PHONY: all main check include lib clean

all : main check

check: lib
	$(MAKE) -C $(CHECKDIR)

main: lib
	$(MAKE) -C $(MAINDIR)

lib : include
	$(MAKE) -C $(LIBDIR)

include:
	$(MAKE) -C $(INCDIR)

clean:
	$(MAKE) -C $(INCDIR) clean
	$(MAKE) -C $(LIBDIR) clean
	$(MAKE) -C $(MAINDIR) clean
	$(MAKE) -C $(CHECKDIR) clean
