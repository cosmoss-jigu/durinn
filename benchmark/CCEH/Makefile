UTILDIR = util
SRCDIR = src
MAINDIR = main

.PHONY: all main src util clean

all : main

main: src
	$(MAKE) -C $(MAINDIR)

src: util
	$(MAKE) -C $(SRCDIR)

util:
	$(MAKE) -C $(UTILDIR)

clean:
	$(MAKE) -C $(UTILDIR) clean
	$(MAKE) -C $(SRCDIR) clean
	$(MAKE) -C $(MAINDIR) clean
