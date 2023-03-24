all: a3_asm a3_c

a3_asm:
	as -o main.o main_asm.s
	ld -s -o a3_asm main.o
a3_c:
	gcc main.c -o a3_c -Wall -Wextra -Wpedantic
clean:
	rm *.o a3_asm a3_c
