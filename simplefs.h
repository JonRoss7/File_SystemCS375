#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BLOCK_SIZE 4096
#define MAX_BLOCKS 1024
#define MAX_INODES 128
#define MAX_NAME 256
#define MAX_LOGS 100

struct inode {
    int id;
    int size;
    char permissions[10];
    int ref_count;
    int blocks[12];
    int indirect_block;
    int owner_uid;
    int group_id;
    time_t timestamp;
};

struct dir_entry {
    char name[MAX_NAME];
    int inode_id;
    int is_soft_link;
    char link_path[MAX_NAME];
};

struct log_entry {
    char operation[MAX_NAME];
    int related_inode_id;
    time_t timestamp;
    unsigned int hash;
};

struct simplefs {
    char blocks[MAX_BLOCKS][BLOCK_SIZE];
    struct inode inodes[MAX_INODES];
    struct dir_entry directory[MAX_INODES];
    struct log_entry logs[MAX_LOGS];
    int block_count;
    int inode_count;
    int dir_count;
    int log_count;
};

void init_fs(struct simplefs *fs);
int create_file(struct simplefs *fs, const char *name, const char *permissions, int uid, int gid, const char *data);
int read_file(struct simplefs *fs, const char *name, int uid, int gid, char *buffer, int max_len);
int create_hard_link(struct simplefs *fs, const char *existing_name, const char *new_name, int uid);
int create_soft_link(struct simplefs *fs, const char *existing_name, const char *new_name, int uid);
void print_logs(struct simplefs *fs);

#endif