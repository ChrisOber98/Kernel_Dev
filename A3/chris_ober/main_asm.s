.global _start

_start:
	#my syscall
	mov x8, #451
	ldr x0, =buffer
	mov x1, buffer_len
	svc #0
	cmp x0, #0
	b.lt .out
	mov x2, x0
	mov x8, #64
	mov x0, #1
	ldr x1, =buffer
	svc #0
	cmp x0, #0
	b.lt .out
	mov x0, #0
.out:
	mov x8, #93
	svc #0

.section .data
message:
buffer: .space 64
.equ buffer_len, .-buffer
