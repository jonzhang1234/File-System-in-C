// Inode manipulation routines.
//
// Feel free to use as inspiration. Provided as-is.

// based on cs3650 starter code
#ifndef INODE_H
#define INODE_H

#include "blocks.h"
#include "bitmap.h"


// size = 20, round up to 32 just in case we add stuff later
typedef struct inode {
  int refs;  // reference count
  int mode;  // permission & type
  int size;  // bytes
  int direct_block_idx; // directly open specific data blocks to access file data
  int indirect_block_idx; // indirectly granting access to more file data
} inode_t;

void print_inode(inode_t *node);
inode_t *get_inode(int inum);
int get_inum(inode_t *node);
int alloc_inode();
void free_inode(int num);
int grow_inode(inode_t *node, int size);
int shrink_inode(inode_t *node, int size);
int inode_get_bnum(inode_t *node, int file_bnum);

#endif
