#include "simplefs.h"

unsigned int calculate_hash(const char *str, int id, time_t ts) {
    unsigned int str_sum = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        str_sum += (unsigned char)str[i];
    }
    return (unsigned int)(id ^ ts ^ str_sum);
}

void init_fs(struct simplefs *fs) {
    memset(fs, 0, sizeof(struct simplefs));
    fs->block_count = 0;
    fs->inode_count = 0;
    fs->dir_count = 0;
    fs->log_count = 0;
}

int create_file(struct simplefs *fs, const char *name, const char *permissions, int uid, int gid, const char *data) {
    if (fs->inode_count >= MAX_INODES || fs->block_count >= MAX_BLOCKS) return -1;
    
    struct inode *inode = &fs->inodes[fs->inode_count];
    inode->id = fs->inode_count++;
    inode->size = strlen(data);
    strncpy(inode->permissions, permissions, 10);
    inode->ref_count = 1;
    inode->owner_uid = uid;
    inode->group_id = gid;
    inode->timestamp = time(NULL);

    int num_blocks_needed = (inode->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int data_offset = 0;

    for (int i = 0; i < num_blocks_needed && i < 12; i++) {
        int block_id = fs->block_count++;
        inode->blocks[i] = block_id;
        
        for (int j = 0; j < BLOCK_SIZE && data_offset < inode->size; j++) {
            fs->blocks[block_id][j] = data[data_offset++] ^ 0x55;
        }
    }

    if (num_blocks_needed > 12) {
        int indirect_blk_id = fs->block_count++;
        inode->indirect_block = indirect_blk_id;
        
        int *indirect_pointers = (int *)fs->blocks[indirect_blk_id];

        for (int i = 12; i < num_blocks_needed; i++) {
            int block_id = fs->block_count++;
            indirect_pointers[i - 12] = block_id;

            for (int j = 0; j < BLOCK_SIZE && data_offset < inode->size; j++) {
                fs->blocks[block_id][j] = data[data_offset++] ^ 0x55;
            }
        }
    }

    struct dir_entry *entry = &fs->directory[fs->dir_count++];
    strncpy(entry->name, name, MAX_NAME);
    entry->inode_id = inode->id;
    entry->is_soft_link = 0;

    struct log_entry *log = &fs->logs[fs->log_count % MAX_LOGS];
    snprintf(log->operation, MAX_NAME, "Created file %s (UID:%d GID:%d)", name, uid, gid);
    log->timestamp = time(NULL);
    log->related_inode_id = inode->id;
    log->hash = calculate_hash(log->operation, log->related_inode_id, log->timestamp);
    fs->log_count++;

    return inode->id;
}

int read_file(struct simplefs *fs, const char *name, int uid, int gid, char *buffer, int max_len) {
    for (int i = 0; i < fs->dir_count; i++) {
        if (strcmp(fs->directory[i].name, name) == 0) {
            
            if (fs->directory[i].is_soft_link) {
                 return read_file(fs, fs->directory[i].link_path, uid, gid, buffer, max_len);
            }

            struct inode *inode = &fs->inodes[fs->directory[i].inode_id];

            int can_read = 0;
            if (uid == inode->owner_uid) {
                if (inode->permissions[0] == 'r') can_read = 1;
            } else if (gid == inode->group_id) {
                if (inode->permissions[3] == 'r') can_read = 1;
            } else {
                if (inode->permissions[6] == 'r') can_read = 1;
            }

            if (!can_read) return -1;

            int bytes_read = 0;
            int block_idx = 0;
            
            while (bytes_read < inode->size && bytes_read < max_len) {
                int actual_block_id;

                if (block_idx < 12) {
                    actual_block_id = inode->blocks[block_idx];
                } else {
                    int *indirect_pointers = (int *)fs->blocks[inode->indirect_block];
                    actual_block_id = indirect_pointers[block_idx - 12];
                }

                for (int j = 0; j < BLOCK_SIZE && bytes_read < inode->size && bytes_read < max_len; j++) {
                    buffer[bytes_read++] = fs->blocks[actual_block_id][j] ^ 0x55;
                }
                block_idx++;
            }
            buffer[bytes_read] = '\0';

            struct log_entry *log = &fs->logs[fs->log_count % MAX_LOGS];
            snprintf(log->operation, MAX_NAME, "Read file %s (UID:%d GID:%d)", name, uid, gid);
            log->timestamp = time(NULL);
            log->related_inode_id = inode->id;
            log->hash = calculate_hash(log->operation, log->related_inode_id, log->timestamp);
            fs->log_count++;

            return bytes_read;
        }
    }
    return -1; 
}

int create_hard_link(struct simplefs *fs, const char *existing_name, const char *new_name, int uid) {
    for (int i = 0; i < fs->dir_count; i++) {
        if (strcmp(fs->directory[i].name, existing_name) == 0) {
            struct inode *inode = &fs->inodes[fs->directory[i].inode_id];
            
            if (inode->owner_uid != uid && !(inode->permissions[7] == 'w')) return -1;

            inode->ref_count++;
            
            struct dir_entry *entry = &fs->directory[fs->dir_count++];
            strncpy(entry->name, new_name, MAX_NAME);
            entry->inode_id = inode->id;
            entry->is_soft_link = 0;

            struct log_entry *log = &fs->logs[fs->log_count % MAX_LOGS];
            snprintf(log->operation, MAX_NAME, "Created hard link %s to %s by UID %d", new_name, existing_name, uid);
            log->timestamp = time(NULL);
            log->related_inode_id = inode->id;
            log->hash = calculate_hash(log->operation, log->related_inode_id, log->timestamp);
            fs->log_count++;
            
            return 0;
        }
    }
    return -1;
}

int create_soft_link(struct simplefs *fs, const char *existing_name, const char *new_name, int uid) {
    struct dir_entry *entry = &fs->directory[fs->dir_count++];
    strncpy(entry->name, new_name, MAX_NAME);
    strncpy(entry->link_path, existing_name, MAX_NAME);
    entry->is_soft_link = 1;

    struct log_entry *log = &fs->logs[fs->log_count % MAX_LOGS];
    snprintf(log->operation, MAX_NAME, "Created soft link %s to %s by UID %d", new_name, existing_name, uid);
    log->timestamp = time(NULL);
    log->related_inode_id = -1;
    log->hash = calculate_hash(log->operation, -1, log->timestamp);
    fs->log_count++;
    
    return 0;
}

void print_logs(struct simplefs *fs) {
    printf("\n--- Filesystem Logs (Integrity Check) ---\n");
    for (int i = 0; i < fs->log_count && i < MAX_LOGS; i++) {
        struct log_entry *log = &fs->logs[i];
        
        unsigned int calculated = calculate_hash(log->operation, log->related_inode_id, log->timestamp);
        
        char status[20] = "OK";
        if (calculated != log->hash) {
            strcpy(status, "TAMPERED!");
        }

        printf("[%ld] %s | Hash: %u | Status: %s\n", 
               log->timestamp, log->operation, log->hash, status);
    }
}