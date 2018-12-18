#define SECTOR_SIZE 512

int get_total_disk_size(char* ptr);
int get_fat_entry(int n, char* ptr);
int get_free_disk_size(int disk_size, char* ptr);
