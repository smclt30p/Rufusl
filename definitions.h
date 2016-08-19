#ifndef DEFINITIONS
#define DEFINITIONS

#define FS_FAT32 0
#define FS_NTFS 1

#define FS_FAT32_LABEL "FAT32"
#define FS_NTFS_LABEL "NTFS"

#define BS_512B 0
#define BS_1024B 1
#define BS_2048B 2
#define BS_4096B 3
#define BS_8192B 4
#define BS_16384B 5
#define BS_32768B 6

#define BS_512B_LABEL "512 bytes"
#define BS_1024B_LABEL "1024 bytes"
#define BS_2048B_LABEL "2048 bytes"
#define BS_4096B_LABEL "4096 bytes"
#define BS_8192B_LABEL "8192 bytes"
#define BS_16384B_LABEL "16384 bytes"
#define BS_32768B_LABEL "32768 bytes"

#define TB_MBR 0
#define TB_GPT 1

#define TB_MBR_LABEL "MBR Partition table for BIOS"
#define TB_GPT_LABEL "GPT Partition table for UEFI"

#define SRC_ISO 0
#define SRC_DD 1

#define SRC_ISO_LABEL "ISO Image"
#define SRC_DD_LABEL "DD Image"

#define MOUNT_FAT32 "vfat"
#define MOUNT_NTFS "ntfs"
#define MOUNT_LOOP "iso9660"

#endif // DEFINITIONS

