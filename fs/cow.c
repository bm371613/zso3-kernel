#include <linux/syscalls.h>

SYSCALL_DEFINE2(cow, int, srcfd, int, dstfd)
{
	return 0;
}
