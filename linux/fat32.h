#define BPB_SEC_PER_CLUS_OFFSET 13
#define BPB_TOT_SEC_32_OFFSET 32
#define BPB_FAT_SZ_32_OFFSET 36
#define BPB_LABEL_OFFSET 71

#define SEEKNWRITE(fd, offset, array, max) \
    lseek(fd, offset, SEEK_SET); \
    if (write(fd, array, max) < 0) { \
        perror("write"); \
        return -1; \
    } \

int format_fat32(const uint32_t *part_fd, uint8_t cluster_size, char *label);
