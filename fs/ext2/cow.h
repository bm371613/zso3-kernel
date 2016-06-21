#include <linux/fs.h>

void ext2_cow_remove(struct inode *inode);
int ext2_cow_inode(struct inode *src, struct inode *dst);
int ext2_cow_shared(struct inode *inode, unsigned long block);
