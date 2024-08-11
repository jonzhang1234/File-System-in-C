#include "directory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// returns the pointer to the inode of the created directory
inode_t *directory_init() {
    inode_t* rootnode;
    rootnode = get_inode(alloc_inode());
    rootnode->mode = 040755; // directory mode
    rootnode->refs = 1;
    rootnode->size = 0;
    rootnode->direct_block_idx = alloc_block();
    return rootnode;
}

// returns the inum of the directory if found, -1 if not
int directory_lookup(inode_t* di, const char* name) {
    dirent_t *dir_list = (dirent_t *)blocks_get_block(di->direct_block_idx);
    for (int i = 0; i < di->size/sizeof(dirent_t); i++) {
        dirent_t *dir = dir_list + i;
        printf("DIR_NAME: %s\n", dir->name);
        if (dir != NULL && strcmp(dir->name, name) == 0) {
            return dir->inum;
        }
    }

    return -1;
}

int tree_lookup(const char *path) {
    printf("\n");
    printf("TREE_LOOKUP IS GETTING USED");
    printf("\n");
    if (strcmp(path, "/") == 0 || strlen(path) == 0) {
        return 0; //root dir inum
    }

    slist_t *path_list = slist_explode(++path, '/');

    int inum = 0;
    for (slist_t *item = path_list; item != NULL; item = item->next) {
        inum = directory_lookup(get_inode(inum), item->data);
        if (inum == -1) {
            return -1;
        }
    }

    return inum;
}

// puts a new dirent at the end of the directory of the given inode
int directory_put(inode_t *di, const char *name, int inum) {
    dirent_t *new_dir = (dirent_t *) (blocks_get_block(di->direct_block_idx) + di->size);
    grow_inode(di, sizeof(dirent_t));
    strncpy(new_dir->name, name, DIR_NAME_LENGTH);
    new_dir->name[strlen(name)] = '\0';
    new_dir->inum = inum;

    return 0;
}

//helper to shift dirs down 1 spot
void shift_dirs(dirent_t *dir_list, int dir_idx, int num_dirs) {
    for (int i = dir_idx; i < num_dirs - 1; i++){
        dirent_t new_dir;

        new_dir.inum = dir_list[i+1].inum;
        strcpy(new_dir.name, dir_list[i+1].name);
        new_dir.name[strlen(dir_list[i+1].name)] = '\0';

        *(dir_list + i) = new_dir;
    }
}

// finds the dirent, deletes it and its node, and shifts all later dirents down one
// returns 0 on success, -1 otherwise
int directory_delete(inode_t *di, const char *name){
    dirent_t* dirs = blocks_get_block(di->direct_block_idx);
    int num_dirs = di->size/sizeof(dirent_t);
    for(int i = 0; i < num_dirs; i++) {
        if(strcmp(name, dirs[i].name) == 0){ // if the directory name is found
            free_inode(dirs[i].inum);
            shift_dirs(dirs, i, num_dirs);
            shrink_inode(di, sizeof(dirent_t));
            return 0;
        }
    }
    return -1;
}

// list the subdirectories in path
slist_t *directory_list(const char *path){
    printf("dir list: %s", path);
    int inum = tree_lookup(path);
    inode_t *dir_node = get_inode(inum);
    if (dir_node->size == 0) {
        return NULL;
    }
    dirent_t* dir = (dirent_t *) blocks_get_block(dir_node->direct_block_idx);
    slist_t *d_list = NULL;

    for(int i = 0; i < dir_node->size/sizeof(dirent_t); i++) {
        printf("Entry %d: Name: %s, Inum: %d\n", i + 1, dir->name, dir->inum);
        if (dir != NULL) {
            d_list = slist_cons(dir->name, d_list);
        }
        dir += 1;
    }
    return d_list;
}

void print_directory(inode_t *dd){
    dirent_t* dirs = blocks_get_block(dd->direct_block_idx);

    for(int i = 0; i < dd->size/sizeof(dirent_t); i++) {
        if(dirs[i].inum != 0) printf("%s\n", dirs[i].name);
    }
}

const char *get_file_name(const char *path) {
    slist_t *path_names = slist_explode(path, '/');
    int len = slist_len(path_names);
    const char *name = slist_get(path_names, len - 1)->data;
    return name;
}

const char *get_parent_path(const char *path, const char *file_name) {
    int len = strlen(path) - strlen(file_name);
    char *parent_path = (char *) malloc(len + 1);
    strncpy(parent_path, path, len);
    parent_path[len] = '\0';
    return parent_path;
}