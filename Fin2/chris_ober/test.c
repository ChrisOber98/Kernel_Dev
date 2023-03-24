#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct pig_ioctl_args {
	char *buff;
	size_t buff_sz;
};

#define DEV_FILE "/dev/igpay"
#define IOC_PIG_ENCODE_MSG _IO(0x11, 0)
#define IOC_PIG_GET_ORIG   _IOR(0x11, 1, struct pig_ioctl_args)
#define IOC_PIG_RESET      _IO(0x11, 2)
#define IOC_PIG_MSG_LEN    _IOR(0x11, 3, size_t)
#define IOC_PIG_NUM_TRANS  _IOR(0x11, 4, size_t)

//#define TEST_STR "a pig latin test string"
//#define TEST_STR_ENCODE1 "ayay igpay atinlay esttay ringstay"
//#define TEST_STR_ENCODE2 "ayayyay igpayyay atinlayyay ingstayray"

#define TEST_STR "doggy"
#define TEST_STR_ENCODE1 "oggyday"
#define TEST_STR_ENCODE2 "ggydayoay"

#define TEST_STR2 "cheese"
#define TEST_STR_2_ENCODE1 "heesecay"

int main(void)
{
	printf("Beginning of testing.\n");


	printf("trying to test open\n");

	int fd = open(DEV_FILE, O_RDWR);

	if (fd < 0)
		err(1, "unable to open file");
	else
		printf("open tests good\n\n");


	printf("trying to test read initial with nothing\n");
	char buff[1024];
	{
		ssize_t ret = read(fd, buff, sizeof(buff));

		if (ret < 0)
			err(1, "unable to read from device");
		if (ret != 0)
			warnx(
				"initial read test failed, expected 0, got %zd",
				ret);
		if(ret == 0)
			printf("initial read test good\n\n");
	}
	{
		struct pig_ioctl_args args;
		ssize_t ret = write(fd, TEST_STR, sizeof(TEST_STR));

		if (ret < 0)
			err(1, "Unable to write to device");
		if (ret != sizeof(TEST_STR))
			warnx(
				"expected write to consume all %lu bytes, got %zd",
				sizeof(TEST_STR),
				ret);
		if (ret == sizeof(TEST_STR))
			printf("write test good\n\n");

		printf("trying to test ioctl get orig\n");
		args.buff = NULL;
		ret = ioctl(fd, IOC_PIG_GET_ORIG, &args);
		if (ret == 0)
			err(1, "expected error return from NULL input");
		if (ret != 0)
			printf("ioctl test to check NULL input sucess\n\n");

		args.buff_sz = 1024;
		args.buff = (char *) malloc(args.buff_sz);
		if (args.buff == NULL) {
			printf("malloc failed\n");
			exit(-1);
		}

		printf("trying to test ioctl get orig\n");
		ret = ioctl(fd, IOC_PIG_GET_ORIG, &args);
		if (ret < 0)
			err(1, "unable to read from device");
		if (memcmp(TEST_STR, args.buff, sizeof(TEST_STR)) != 0)
			warnx(
				"expected %s, got %s",
				TEST_STR,
				args.buff);
		if (memcmp(TEST_STR, args.buff, sizeof(TEST_STR)) == 0)
			printf("ioctl test to check get orig input sucess\n\n");
		printf("Read out %s\n\n", args.buff);

	}
	{
		printf("Starting new read test\n");
		ssize_t ret = read(fd, buff, sizeof(buff));

		if (ret < 0)
			err(1, "unable to read from device");
		if (ret != sizeof(TEST_STR_ENCODE1))
			warnx(
				"read test failed, expected %lu, got %zd",
				sizeof(TEST_STR_ENCODE1),
				ret);
		if (ret == sizeof(TEST_STR_ENCODE1))
			printf("read test sucess\n\n");

		printf("trying to compare\n");
		if (memcmp(TEST_STR_ENCODE1, buff, sizeof(TEST_STR_ENCODE1)) != 0)
			warnx(
				"expected pig latin-ized string, got %s",
				buff);
		if (memcmp(TEST_STR_ENCODE1, buff, sizeof(TEST_STR_ENCODE1)) == 0)
			printf("compare test sucess\n\n");
		printf("Read out %s\n\n", buff);

		printf("new ioctl test\n");

		ret = ioctl(fd, IOC_PIG_ENCODE_MSG, NULL);

		if (ret < 0)
			err(1, "unable to perform ioctl on device");
		if (ret != 0)
			warnx("expected return of 0 from ioctl got %zd", ret);
		if (ret == 0)
			printf("Ioctl encode msg performed.\n\n");

		off_t pos = lseek(fd, 0, SEEK_SET);

		if (pos != 0)
			warnx("expected 2, got %ld", pos);


		printf("Test double encode msg.\n");
		ret = read(fd, buff, sizeof(buff));

		if (ret < 0)
			err(1, "unable to read from device");
		if (ret != sizeof(TEST_STR_ENCODE2))
			warnx(
				"read test failed, expected %lu, got %zd",
				sizeof(TEST_STR_ENCODE2),
				ret);
		if (memcmp(TEST_STR_ENCODE2, buff, sizeof(TEST_STR_ENCODE2)) != 0)
			warnx(
				"expected doubly-encoded string, got %s",
				buff);
		printf("Read out %s\n\n", buff);
	}
	{
		///////////////
		size_t size = 1337;
		ssize_t ret = ioctl(fd, IOC_PIG_MSG_LEN, &size);

		if (ret < 0)
			err(1, "unable to perform ioctl on device");
		if (ret != 0)
			warnx("expected return of 0 from ioctl got %zd", ret);
		if (ret == 0)
			printf("Passed msg len\n");
		if (size != sizeof(TEST_STR_ENCODE2))
			warnx(
				"expected size of %lu, got %zu",
				sizeof(TEST_STR_ENCODE2),
				size);
		if (size == sizeof(TEST_STR_ENCODE2))
			printf("got correct size\n\n");


		printf("Trying to write to device again.\n");
		ret = write(fd, TEST_STR2, sizeof(TEST_STR2));

		if (ret < 0)
			err(1, "Unable to write to device");
		if (ret != sizeof(TEST_STR2))
			warnx(
				"expected write to consume all %lu bytes, got %zd",
				sizeof(TEST_STR2),
				ret);
		if (ret == sizeof(TEST_STR2))
			printf("Passed writing to device again.\n\n");


		printf("Trying to find number of translations device again.\n");
		size = 1337;
		ret = ioctl(fd, IOC_PIG_NUM_TRANS, &size);

		if (ret < 0)
			err(1, "unable to perform ioctl on device");
		if (ret != 0)
			warnx("expected return of 0 from ioctl got %zd", ret);
		if (size != 3)
			warnx(
				"expected 3 translations total, got %zu",
				size);
		if (size == 3)
			printf("got correct trans\n\n");
	}
	{
		int fd2 = open(DEV_FILE, O_RDONLY);

		if (fd2 < 0)
			err(1, "unable to open device");

		printf("Trying to read fd2\n");
		ssize_t ret = read(fd2, buff, sizeof(buff));

		if (ret < 0)
			err(1, "unable to read into buff");
		if (ret != 9)
			warnx(
				"expected to read %lu bytes, got %zd",
				sizeof(TEST_STR_2_ENCODE1),
				ret);

		close(fd2);
	}
	{
		printf("Testing ioctl reset");
		ssize_t ret = ioctl(fd, IOC_PIG_RESET, NULL);

		if (ret < 0)
			err(1, "unable to perform ioctl on device");
		if (ret != 0)
			warnx("expected return of 0 from ioctl got %zd", ret);
		if (ret == 0)
			printf("Ioctl reset happened\n");

		ret = read(fd, buff, sizeof(buff));

		if (ret < 0)
			err(1, "unable to read from device");
		if (ret != 0)
			warnx(
				"test failed, expected 0, got %zd",
				ret);
		if (ret == 0)
			printf("Ioctl reset worked\n\n");
	}
	printf("End of testing.\n");
	close(fd);

	return 0;
}
