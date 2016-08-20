#ifndef ISO_H
#define ISO_H

#include <stdint.h>

#define LABEL_OFFSET 0x8028
#define FAT32_MAX 4294967296LL
#define TEMP_DIR_ISO "/mnt/rufus_isofs"

typedef struct iso_info {

    uint8_t has_syslinux;
    uint8_t has_grub;
    uint8_t has_windows;
    uint8_t has_arch;
    uint8_t has_uefi;
    uint8_t has_autorun;
    uint8_t has_4gb;

    char label[11];

} iso_info_t;

int recursive_iso_scan(uint32_t *loop_fds);

#endif // ISO_H
