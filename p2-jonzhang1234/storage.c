#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "blocks.h"
#include "directory.h"
#include "storage.h"


void storage_init(const char *path) {
    blocks_init(path); // allocates block for both bitmaps already

    void *bbm = get_blocks_bitmap();
    //need space for at least 100 inodes, should fit in 2 blocks
    for (int i = 1; i <= INODE_BLOCKS; i++) {
        if (!bitmap_get(bbm, i)) {
            int inode_block = alloc_block();
        }
    }
    //initialize root directory
    if (!bitmap_get(bbm, INODE_BLOCKS+1)) {
        directory_init();
    }
}

// returns 0 on success, -1 otherwise
int storage_stat(const char *path, struct stat *st) {
    int inum = tree_lookup(path);
    if (inum == -1) {
        return -1;
    }
    inode_t *dir_node = get_inode(inum);
    printf("Stat: ");
    print_inode(dir_node);
    if (dir_node != NULL) {
        st->st_mode = dir_node->mode;
        st->st_size = dir_node->size;
        st->st_nlink = dir_node->refs;
        st->st_uid = getuid();
        return 0;
    }
    return -1;
}

int storage_read(const char *path, char *buf, size_t size, off_t offset) {
    inode_t *dir_node = get_inode(tree_lookup(path));
    size_t space_read = 0;
    if (dir_node != NULL) {
        if (offset >= dir_node->size) return 0;
        if (size >= dir_node->size - offset) { space_read = dir_node->size - offset; }
        else { space_read = size; }
        memcpy(buf, blocks_get_block(dir_node->direct_block_idx) + offset, space_read);
        return space_read;
    }
    return -1;
}

int storage_write(const char *path, const char *buf, size_t size, off_t offset){
    inode_t *dir_node = get_inode(tree_lookup(path));
    size_t space_write = 0;
    if(dir_node != NULL){
        if (offset + size > dir_node->size) {
            //need grow inode
            grow_inode(dir_node, offset + size - dir_node->size);
        }
        if(size >= dir_node->size - offset) space_write = dir_node->size - offset;
        else space_write = size;
        memcpy(blocks_get_block(dir_node->direct_block_idx) + offset, buf, space_write);

        return space_write;
    }
    return -1;
}

int storage_truncate(const char *path, off_t size) {
    int inum = tree_lookup(path);
    if (inum == -1) {
        return -ENOENT;
    }

    inode_t *dir_inode = get_inode(inum);
    int diff = size - dir_inode->size;
    if (diff > 0) {
        grow_inode(dir_inode, diff);
    } else if (diff < 0) {
        shrink_inode(dir_inode, -1 * diff);
    }

    if (dir_inode->size == 0) {
        free_inode(inum);
    }

    return 0;
}

// returns 0 on success, -1 or error code otherwise
int storage_mknod(const char *path, int mode) {
    printf("storage_mknod\n");
    const char *file_name = get_file_name(path);
    const char *parent_path = get_parent_path(path, file_name);

    int inum = tree_lookup(parent_path);
    if (inum == -1) {
        return -ENOENT;
    }

    inode_t *parent_inode = get_inode(inum);
    printf("PARENT_INODE STATS:");
    print_inode(parent_inode);
    int new_inum = alloc_inode();
    printf("storage_mknod:  Allocated new inode: %d\n", new_inum);

    inode_t *new_node = get_inode(new_inum);
    new_node->mode = mode;

    directory_put(parent_inode, file_name, new_inum);
    return 0;
}

// decrements reference count of the inode of the given path and frees
// inode and its block pointers if becomes 0
// returns 0 on success, -1 otherwise
int storage_unlink(const char *path) {
    printf("storage_unlink\n");
    const char *file_name = get_file_name(path);
    const char *parent_path = get_parent_path(path, file_name);

    int inum = tree_lookup(parent_path);
    if (inum == -1) {
        return -ENOENT;
    }

    inode_t *parent_node = get_inode(inum);

    assert(directory_delete(parent_node, file_name) == 0);

    inum = tree_lookup(path);
    inode_t *dir_node = get_inode(inum);
    dir_node->refs--;

    if (dir_node->refs == 0) {
        free_inode(get_inum(dir_node));
    }

    return 0;
}


int storage_link(const char *from, const char *to) {
    printf("storage_link\n");
    const char *file_name = get_file_name(to);
    const char *parent_path = get_parent_path(to, file_name);

    int parent_inum = tree_lookup(parent_path);
    int from_inum = tree_lookup(from);

    if (parent_inum == -1 || from_inum == -1) {
        return -ENOENT;
    }

    inode_t *to_parent_inode = get_inode(parent_inum);
    inode_t *from_inode = get_inode(from_inum);
    directory_put(to_parent_inode, file_name, get_inum(from_inode));
    from_inode->refs++;

    return 0;
}
int storage_rename(const char *from, const char *to) {
    printf("storage_rename\n");
    assert(storage_link(from, to) == 0);
    assert(storage_unlink(from) == 0);

    return 0;
}