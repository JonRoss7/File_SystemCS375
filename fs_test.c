#include "simplefs.h"

#define BUFFER_SIZE 60000 

int main() {
    struct simplefs fs;
    init_fs(&fs);
    char buffer[BUFFER_SIZE];

    printf("1. Testing Indirect Blocks (Creating 50KB file)...\n");
    char *large_data = malloc(50000); 
    memset(large_data, 'A', 49999);
    large_data[49999] = '\0';
    
    create_file(&fs, "largefile.txt", "rw-r--r--", 1000, 100, large_data);
    
    int bytes = read_file(&fs, "largefile.txt", 1000, 100, buffer, BUFFER_SIZE);
    if (bytes > 4096 * 12) {
        printf("   SUCCESS: Read %d bytes (Indirect block used).\n", bytes);
    } else {
        printf("   FAIL: Could not read full large file.\n");
    }
    free(large_data);

    printf("\n2. Testing Group Permissions...\n");
    create_file(&fs, "secret.txt", "rw-r-----", 1000, 100, "Group Secret Data");

    if (read_file(&fs, "secret.txt", 1000, 999, buffer, BUFFER_SIZE) > 0)
        printf("   Owner Access: Allowed (Correct)\n");
    else
        printf("   Owner Access: Denied (Incorrect)\n");

    if (read_file(&fs, "secret.txt", 1002, 100, buffer, BUFFER_SIZE) > 0)
        printf("   Group Access: Allowed (Correct)\n");
    else
        printf("   Group Access: Denied (Incorrect)\n");

    if (read_file(&fs, "secret.txt", 1003, 101, buffer, BUFFER_SIZE) < 0)
        printf("   Other Access: Denied (Correct)\n");
    else
        printf("   Other Access: Allowed (Incorrect! Security Breach)\n");


    printf("\n3. Testing Log Verification...\n");
    
    printf("   [Attacker] Modifying log entry in memory...\n");
    strcpy(fs.logs[0].operation, "HACKED FILE ACCESS");

    print_logs(&fs);

    return 0;
}