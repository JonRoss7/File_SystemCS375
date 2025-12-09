# SimpleFS - A Simple File System Implementation

A lightweight in-memory file system implementation in C that demonstrates fundamental file system concepts including inode management, permission control, indirect blocks, hard/soft links, and log integrity verification.

## Features

### Core Functionality
- **Inode-based Architecture**: Files are managed through inodes with metadata (size, permissions, timestamps, ownership)
- **Block Storage**: 4KB block size with support for up to 1024 blocks
- **Direct & Indirect Blocks**: 
  - 12 direct block pointers per inode
  - Single indirect block for larger files (>48KB)
- **Directory Entries**: Named file references with support for both files and links

### Security & Permissions
- **Unix-style Permissions**: Owner, group, and other permission bits (e.g., `rw-r--r--`)
- **Access Control**: Permission checking for read operations based on UID/GID
- **Data Encryption**: XOR-based basic encryption (XOR with 0x55) for stored data

### Advanced Features
- **Hard Links**: Multiple directory entries pointing to the same inode (reference counting)
- **Soft Links (Symbolic Links)**: Name-based links that redirect file access
- **Transaction Logs**: All operations are logged with timestamps
- **Log Integrity**: Hash-based verification to detect log tampering

## File Structure

```
.
├── simplefs.h      # Header file with data structures and function declarations
├── simplefs.c      # Core implementation of file system operations
├── fs_test.c       # Test suite demonstrating features
└── README.md       # This file
```

## Data Structures

### Inode
```c
struct inode {
    int id;                  // Unique inode identifier
    int size;                // File size in bytes
    char permissions[10];    // Unix-style permission string
    int ref_count;           // Number of hard links
    int blocks[12];          // Direct block pointers
    int indirect_block;      // Indirect block pointer
    int owner_uid;           // Owner user ID
    int group_id;            // Group ID
    time_t timestamp;        // Creation/modification time
};
```

### Directory Entry
```c
struct dir_entry {
    char name[MAX_NAME];     // Filename
    int inode_id;            // Associated inode
    int is_soft_link;        // Soft link flag
    char link_path[MAX_NAME];// Soft link target path
};
```

### Log Entry
```c
struct log_entry {
    char operation[MAX_NAME];   // Operation description
    int related_inode_id;       // Related inode
    time_t timestamp;           // Operation timestamp
    unsigned int hash;          // Integrity hash
};
```

## API Functions

### Initialization
```c
void init_fs(struct simplefs *fs);
```
Initializes a new file system instance.

### File Operations
```c
int create_file(struct simplefs *fs, const char *name, const char *permissions, 
                int uid, int gid, const char *data);
```
Creates a new file with specified permissions and data. Returns inode ID on success, -1 on failure.

```c
int read_file(struct simplefs *fs, const char *name, int uid, int gid, 
              char *buffer, int max_len);
```
Reads file contents. Checks permissions and follows soft links. Returns bytes read or -1 on failure.

### Link Operations
```c
int create_hard_link(struct simplefs *fs, const char *existing_name, 
                     const char *new_name, int uid);
```
Creates a hard link to an existing file. Increments reference count.

```c
int create_soft_link(struct simplefs *fs, const char *existing_name, 
                     const char *new_name, int uid);
```
Creates a symbolic link to an existing file.

### Logging
```c
void print_logs(struct simplefs *fs);
```
Displays all logged operations with integrity verification status.

## Building and Running

### Compile
```bash
gcc -o fs_test fs_test.c simplefs.c -Wall
```

### Run Tests
```bash
./fs_test
```

## Test Suite

The `fs_test.c` file demonstrates three key features:

### 1. Indirect Block Testing
Creates a 50KB file to test the indirect block mechanism:
```c
char *large_data = malloc(50000);
create_file(&fs, "largefile.txt", "rw-r--r--", 1000, 100, large_data);
```
- Direct blocks: 12 × 4KB = 48KB
- Remaining data uses indirect block

### 2. Group Permissions
Tests the permission system with different user/group combinations:
```c
create_file(&fs, "secret.txt", "rw-r-----", 1000, 100, "Group Secret Data");
```
- Owner (UID 1000): Read/Write access ✓
- Group member (GID 100): Read access ✓
- Other users: No access ✓

### 3. Log Integrity Verification
Demonstrates tamper detection:
```c
strcpy(fs.logs[0].operation, "HACKED FILE ACCESS");
print_logs(&fs);  // Shows "TAMPERED!" status
```

## Implementation Details

### Block Allocation
- Files are allocated in 4KB blocks
- Direct blocks (0-11) for files ≤ 48KB
- Indirect block contains pointers to additional blocks for larger files

### Permission Checking
Permission string format: `rwxrwxrwx`
- Positions 0-2: Owner (read, write, execute)
- Positions 3-5: Group (read, write, execute)
- Positions 6-8: Other (read, write, execute)

### Data Encryption
Simple XOR encryption for data at rest:
```c
fs->blocks[block_id][j] = data[data_offset++] ^ 0x55;  // Encrypt
buffer[bytes_read++] = fs->blocks[actual_block_id][j] ^ 0x55;  // Decrypt
```

### Log Hashing
Integrity protection using a hash function:
```c
hash = (inode_id) ^ (timestamp) ^ (sum of operation string bytes)
```

## Limitations

- **In-Memory Only**: Data is not persisted to disk
- **Fixed Sizes**: Maximum 1024 blocks, 128 inodes
- **Simple Encryption**: XOR encryption is for demonstration only
- **No Write Permissions Check**: Only read permissions are enforced
- **Single Indirect Block**: Cannot handle files larger than ~1MB