#define MBR "msdos"
#define GPT "gpt"

#define FAT32 "fat32"
#define NTFS "ntfs"

int nuke_and_partition(const char *path_dev, const int table, const int fs);
int full_wipe(const uint32_t *device_fd);
