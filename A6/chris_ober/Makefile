obj-m += echo.o

.PHONY: all build clean load unload

all: test build

test: test.c
	clang -Weverything -Wno-unused-macros -Wno-declaration-after-statement -std=c11 test.c -o test

build:
	make -C /lib/modules/$(shell uname -r)/build modules M=$(shell pwd)

clean:
	-rm test
	make -C /lib/modules/$(shell uname -r)/build clean M=$(shell pwd)

load:
	sudo insmod echo.ko

unload:
	sudo rmmod echo
