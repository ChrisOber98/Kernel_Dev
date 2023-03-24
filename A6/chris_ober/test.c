#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>

#define DEV_FILE "/dev/echo"
#define IOC_ECHO_RESET _IO(0x11, 0)
#define IOC_ECHO_GET_LENGTH _IOR(0x11, 1, size_t)

int main(void)
{
	char buff[1024];

	int fd = open(DEV_FILE, O_RDWR);

	puts("start of testing");
	if(0 > fd)
		err(1, "unable to open file");
	{
		ssize_t ret = write(fd, "Chad", 4);
		if(0 > ret)
			err(1, "Unable to write to device");
		if(4 != ret)
			warnx("expected write to consume all 4 bytes, got %zd", ret);
	}
	{
		ssize_t ret = read(fd, buff, sizeof buff);
		if(0 > ret)
			err(1, "unable to read from device");
		if(4 != ret)
			warnx("initial read test failed, expected 4, got %zd", ret);
		printf("Output string: %.*s\n", (int)ret, buff);
	}
	{
		size_t size = 1337;
		int ret = ioctl(fd, IOC_ECHO_GET_LENGTH, &size);
		if(0 > ret)
			err(1, "unable to preform ioctl on device");
		if(0 != ret)
			warnx("expected return of 0 from ioctl got %d", ret);
		if(4 != size)
			warnx("expected initial size of zero, got %zu", size);
	}
	{
		int fd2 = open(DEV_FILE, O_RDONLY);
		if(0 > fd2)
			err(1, "unable to open device");
		ssize_t ret = read(fd2, buff, sizeof buff);
		if(0 > ret)
			err(1, "unable to read into buff");
		if(4 != ret)
			warnx("expected to read 4 bytes, got %zd", ret);
		if(0 != memcmp("dahC", buff, 4))
			warnx("expected to read original message in reverse, got %.*s", (int)ret, buff);
		off_t pos = lseek(fd2, 2, SEEK_SET);
		if(2 != pos)
			warnx("expected 2, got %ld", pos);
		char c;
		if(1 != read(fd2, &c, 1))
			warnx("expected return of 1");
		if(c != 'h')
			warnx("expected h");
		close(fd2);
	}
	close(fd);
	puts("end of testing");
	return 0;
}
