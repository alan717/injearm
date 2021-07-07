CC	= clang
CFLAGS	= -std=gnu99 -ggdb
UNAME_M := $(shell uname -m)

.PHONY:   arm

all:
ifeq ($(UNAME_M),x86_64)
	$(MAKE) x86_64
endif
ifeq ($(UNAME_M),x86)
	$(MAKE) x64
endif
ifneq (,$(findstring arm,$(UNAME_M)))
	$(MAKE) arm
endif


arm: sample-target sample-library.so
	$(CC) -marm $(CFLAGS) -DARM -o inject  utils.c inject_main.c -ldl


sample-library.so: sample-library.c
	$(CC) $(CFLAGS) -D_GNU_SOURCE -shared -o sample-library.so -fPIC sample-library.c

sample-target: sample-target.c
	$(CC) $(CFLAGS) -o sample-target sample-target.c

clean:

	rm -f inject

