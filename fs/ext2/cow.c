#include <linux/fs.h>
#include <linux/spinlock.h>
#include "cow.h"
#include "ext2.h"


int ext2_cow_inode(struct inode *src, struct inode *dst)
{
	struct inode *next;
	struct ext2_inode_info *src_info = EXT2_I(src),
			       *dst_info = EXT2_I(dst);

	if (src == dst)
		return 0;

	spin_lock(&dst->i_lock);
	spin_lock(&src->i_lock);

	memcpy(dst_info->i_data, src_info->i_data, sizeof(src_info->i_data));
	dst->i_size = src->i_size;
	dst->i_blocks = src->i_blocks;
	dst->i_bytes = src->i_bytes;

	if (src_info->i_cow_next == 0 || src_info->i_cow_next == src->i_ino) {
		src_info->i_cow_next = dst->i_ino;
		src_info->i_cow_prev = dst->i_ino;
		dst_info->i_cow_next = src->i_ino;
		dst_info->i_cow_prev = src->i_ino;
	} else {
		next = ext2_iget(src->i_sb, src_info->i_cow_next);

		spin_lock(&next->i_lock);

		src_info->i_cow_next = dst->i_ino;
		dst_info->i_cow_prev = src->i_ino;
		dst_info->i_cow_next = next->i_ino;
		EXT2_I(next)->i_cow_prev = dst->i_ino;

		spin_unlock(&next->i_lock);

		mark_inode_dirty(next);
		iput(next);
	}

	spin_unlock(&src->i_lock);
	spin_unlock(&dst->i_lock);

	mark_inode_dirty(src);
	mark_inode_dirty(dst);

	return 0;
}
