#include <linux/file.h>
#include <linux/fs.h>
#include <linux/syscalls.h>

static inline struct inode *fd_to_inode(int fd)
{
	struct file *file = __to_fd(__fdget_pos(fd)).file;
	return file ? file->f_mapping->host : 0;
}

SYSCALL_DEFINE2(cow, int, srcfd, int, dstfd)
{
	struct inode *src, *dst;
	struct super_block *sb;

	src = fd_to_inode(srcfd);
	dst = fd_to_inode(dstfd);

	if (!src || !dst)
		return -EBADF;

	sb = src->i_sb;
	if (dst->i_sb != sb)
		return -EXDEV;

	if (!sb->s_op->cow_inode)
		return -EOPNOTSUPP;

	return sb->s_op->cow_inode(src, dst);
}
