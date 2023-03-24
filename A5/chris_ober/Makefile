obj-m += demo.o

.PHONY: build clean load unload

build:
	make -C /lib/modules/$(shell uname -r)/build modules M=$(shell pwd)

clean:
	make -C /lib/modules/$(shell uname -r)/build clean M=$(shell pwd)

load:
	sudo insmod demo.ko

unload:
	sudo rmmod demo
main:
	gcc -o main main.c -Wall -Wextra -Wpedantic

main_clean:
	rm main
