#define TABLE_GPT 1
#define TABLE_MBR 2

#define FS_FAT32 3
#define FS_NTFS 4

#define MBR "msdos"
#define GPT "gpt"

#define FAT32 "fat32"
#define NTFS "ntfs"

int nuke_and_partition(const char *path_dev, const int table, const int fs);
