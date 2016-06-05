#include <linux/fs.h>
#include <linux/spinlock.h>
#include "cow.h"
#include "ext2.h"

int ext2_cow_inode(struct inode *src, struct inode *dst)
{
	struct ext2_inode_info *src_info = EXT2_I(src),
			       *dst_info = EXT2_I(dst);

	spin_lock(&dst->i_lock);
	memcpy(dst_info->i_data, src_info->i_data, sizeof(src_info->i_data));
	dst->i_size = src->i_size;
	dst->i_blocks = src->i_blocks;
	dst->i_bytes = src->i_bytes;
	spin_unlock(&dst->i_lock);

	mark_inode_dirty(dst);
	return 0;
}
