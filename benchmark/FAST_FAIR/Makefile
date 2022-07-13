INCDIR = include
LIBDIR = lib
MAINDIR = main
CHECKDIR = check
TESTDIR = test

.PHONY: all main include lib clean

all : test

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
