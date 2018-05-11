#ifndef _FS_INODE_H_
#define _FS_INODE_H_

#include "../lib/stdint.h"
#include "../lib/kernel/list.h"
#include "../device/ide.h"

struct inode
{
    uint32_t i_no;  // inode编号
    uint32_t i_size; // 此inode为文件时，表示文件的大小。为目录时，表示该目录下所有目录项大小之和 
    uint32_t i_open_cnts; // 文件被打开的次数
    bool write_deny;  // 写文件的标识

    uint32_t i_sectors[13]; // 一个文件只支持13个块，12个直接块，1个间接块
    struct list_elem inode_tag;
};

struct inode *inode_open(struct partition *part, uint32_t inode_no);
void inode_sync(struct partition *part, struct inode *inode, void *io_buf);
void inode_init(uint32_t inode_no, struct inode *new_inode);
void inode_close(struct inode *inode);

#endif // !_FS_INODE_H_