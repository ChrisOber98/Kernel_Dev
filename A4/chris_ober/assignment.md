### line(s) from strace output where your syscall was invoked

gettimeofday({tv_sec=1676700099, tv_usec=202423}, NULL) = 0

0. what arguments if any are being passed to the sycall, what do they do/represent?
What does this syscall do? Why is the program calling it?

It has 2 arguments you can pass:

tv which is a struct timeval that represents seconds and microseconds.
tz which is a struct timezone that represents minutes west of greenwich and dst correction.

You are able to pass these in and will be filled with corresponding information if nothing is passed then it is not set or returned.

### location in the linux kernel source of the defition of your syscall (file and line number)

tools/include/nolibc/sys.h
line 513

1. use git blame to identify who last modified any of the lines in the syscall code.
Who did it, what is their name and email? What is the commit hash for the commit that introduced those changes?
What was the reason for the change? Why do you think it was accepted upstream?

Willy Tarreau, w@1wt.eu, bd8c8fbb866fe.

 The syscall definitions were moved to sys.h. They were arranged
 in a more easily maintainable order, whereby the sys_xxx() and xxx()
 functions were grouped together, which also enlights the occasional
 mappings such as wait relying on wait4().

These were probably accepted in effort to clean up and make this systemm call easier to read.

### output from your bpftrace probe detecting a call to the syscall by the program

ts:0xffff80000d403c70
Kernel Stack:

        ktime_get_real_ts64+0
        invoke_syscall+120
        el0_svc_common.constprop.0+76
        do_el0_svc+52
        el0_svc+52
        el0t_64_sync_handler+244
        el0t_64_sync+404
