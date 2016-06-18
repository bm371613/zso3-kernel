#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/writeback.h>
#include "cow.h"
#include "ext2.h"

void ext2_cow_remove(struct inode *inode) {
	struct ext2_inode_info *info = EXT2_I(inode);
	struct inode *prev, *next;

	if (info->i_cow_next == inode->i_ino)
		return;

	prev = ext2_iget(inode->i_sb, info->i_cow_prev);
	next = ext2_iget(inode->i_sb, info->i_cow_next);

	spin_lock(&prev->i_lock);
	EXT2_I(prev)->i_cow_next = next->i_ino;
	spin_unlock(&prev->i_lock);
	mark_inode_dirty(prev);

	spin_lock(&inode->i_lock);
	EXT2_I(inode)->i_cow_prev = EXT2_I(inode)->i_cow_next = inode->i_ino;
	spin_unlock(&inode->i_lock);
	mark_inode_dirty(inode);

	spin_lock(&next->i_lock);
	EXT2_I(next)->i_cow_prev = prev->i_ino;
	spin_unlock(&next->i_lock);
	mark_inode_dirty(next);

	iput(prev);
	iput(next);
}


void ext2_cow_insert(struct inode *where, struct inode *what) {
	struct ext2_inode_info *where_info = EXT2_I(where),
			*what_info = EXT2_I(what);
	struct inode *next = ext2_iget(where->i_sb, where_info->i_cow_next);

	spin_lock(&what->i_lock);
	what_info->i_cow_prev = where->i_ino;
	what_info->i_cow_next = next->i_ino;
	spin_unlock(&what->i_lock);
	mark_inode_dirty(what);

	spin_lock(&where->i_lock);
	where_info->i_cow_next = what->i_ino;
	spin_unlock(&where->i_lock);
	mark_inode_dirty(where);

	spin_lock(&next->i_lock);
	EXT2_I(next)->i_cow_prev = what->i_ino;
	spin_unlock(&next->i_lock);
	mark_inode_dirty(next);

	iput(next);
}


int ext2_cow_inode(struct inode *src, struct inode *dst)
{
	struct ext2_sb_info *sbi = EXT2_SB(src->i_sb);

	if (src != dst) {
		mutex_lock(&sbi->cow_mutex);

		spin_lock(&dst->i_lock);
		memcpy(EXT2_I(dst)->i_data, EXT2_I(src)->i_data,
				sizeof(EXT2_I(src)->i_data));
		dst->i_size = src->i_size;
		dst->i_blocks = src->i_blocks;
		dst->i_bytes = src->i_bytes;
		spin_unlock(&dst->i_lock);
		mark_inode_dirty(dst);

		ext2_cow_remove(dst);
		ext2_cow_insert(src, dst);

		mutex_unlock(&sbi->cow_mutex);

		invalidate_mapping_pages(dst->i_mapping, 0, -1);
		invalidate_inode_buffers(dst);
		wakeup_flusher_threads(0, WB_REASON_SYNC);
	}

	return 0;
}

int ext2_cow_shared(struct inode *inode, unsigned long block)
{
	struct ext2_inode_info *info = EXT2_I(inode), *other_info;
	struct inode *other;
	unsigned long other_ino = info->i_cow_next,
			block_le = cpu_to_le32(block);
	int i, ret = 0;

	while (other_ino != inode->i_ino) {
		other = ext2_iget(inode->i_sb, other_ino);
		other_info = EXT2_I(other);
		// TODO check indirect
		for (i = 0; i < 15; ++i)
			if (other_info->i_data[i] == block_le) {
				iput(other);
				ret = 1;
				goto out;
			}
		other_ino = other_info->i_cow_next;
		iput(other);
	}
out:
	return ret;
}
