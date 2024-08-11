#include <stdio.h>

#include "inode.h"


void print_inode(inode_t *node) {
    printf("Location: %p\n", node);
    printf("Refs: %d\n", node->refs);
    printf("Node Mode: %d\n", node->mode);
    printf("Node size: %d\n", node->size);
    printf("Direct Pointer: %d\n", node->direct_block_idx);
    printf("Indirect Pointer: %d\n", node->indirect_block_idx);
}

inode_t *get_inode(int inum) {
    inode_t *inodes = blocks_get_block(1); // inode block starts at second block
    return inodes + inum;
}

int get_inum(inode_t *node) {
    return node - get_inode(0);
}


int alloc_inode() {
    void *inbm = get_inode_bitmap();
    int inum;
    for (int i = 0; i < 256; i++) {
        if (!bitmap_get(inbm, i)) { //if free
            bitmap_put(inbm, i, 1);
            inum = i;
            break;
        }
    }
    printf("alloc inode: %d\n", inum);
    inode_t* new_node = get_inode(inum);
    new_node->refs = 1;
    new_node->size = 0;
    new_node->mode = 0;
    new_node->direct_block_idx = alloc_block();

    return inum;
}

// free the inode at the given index
void free_inode(int num) {
    inode_t *node = get_inode(num);
    if (node->direct_block_idx != 0) free_block(node->direct_block_idx); // deallocate blocks
    bitmap_put(get_inode_bitmap(), num, 0); // mark inode to be free in bitmap
}

// grows the inode and allocates more blocks if we need more space
int grow_inode(inode_t *node, int size) {
    printf("Before growth - Node size: %d, New block size: %d\n", node->size, size);

    int remainder = node->size % BLOCK_SIZE;
    int new_block_size = size + remainder;

    printf("Node size: %d, New block size: %d\n", node->size, new_block_size);

    if (new_block_size > BLOCK_SIZE) {
        if (node->indirect_block_idx != 0) {
            int *block_ptr = blocks_get_block(node->indirect_block_idx);
            for (int i = 0; *block_ptr != 0; i++) {
                if (i == BLOCK_SIZE/sizeof(int)) { // too big
                    return -1;
                }
                block_ptr += 1;
            }
            *block_ptr = alloc_block();
        }
        else {
            node->indirect_block_idx = alloc_block();
            int *block_ptr = blocks_get_block(node->indirect_block_idx);
            *block_ptr = alloc_block();
        }
    }

    node->size += size;
    printf("After growth - Node size: %d\n", node->size);
    return node->size;
}



// shrinks the inode and deallocates blocks if not needed anymore
int shrink_inode(inode_t *node, int size){
    int remainder = node->size % BLOCK_SIZE;
    int new_block_size = remainder - size;
    if (new_block_size <= 0) {
        if (node->indirect_block_idx != 0) {
            int *block_ptr = blocks_get_block(node->indirect_block_idx);
            for (int i = 0; *block_ptr != 0; i++) {
                if (i == BLOCK_SIZE/ sizeof(int)) { // too big
                    return -1;
                }
                block_ptr += 1;
            }
            free_block(*(block_ptr - 1));
        }
    }
    node->size -= size;
    return 0;
}

// gets the block number for the inode
int inode_get_bnum(inode_t *node, int file_bnum){
    if (file_bnum == node->direct_block_idx) {
        return node->direct_block_idx;
    } else {
        int* iptrs = blocks_get_block(node->indirect_block_idx);
        return *iptrs;
    }
}