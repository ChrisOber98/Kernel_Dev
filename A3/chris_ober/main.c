#include <unistd.h>

int main(void)
{
	char buf[1000];
	size_t len = 1000;

	long length_of_call;
	length_of_call = syscall(451, buf, len);

	if (length_of_call < 0)
		syscall(93, length_of_call);

	long val;
	val = syscall(64, 1, buf, length_of_call);

	if (val < 1)
		syscall(93, val);

	return 0;
}
