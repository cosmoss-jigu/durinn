DIR = ../../../../third_party/NVTraverse/BST
FILES = $(DIR)/Aravind/BSTAravindTraverse-ciri.h \
					$(DIR)/new_pmem_utils.h \
					$(DIR)/pmem_utils.h \
					$(DIR)/utils.h \
					$(DIR)/../pmdk.h

.PHONY: all copy clean

all: copy

copy:
	@ cp $(FILES) .

clean:
	@ rm -f *.h
