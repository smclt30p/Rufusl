#ifndef ISO_H
#define ISO_H

#include <stdint.h>
#include "rufusl.h"

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

typedef struct {
    char label[192];			/* 3*64 to account for UTF-8 */
    char usb_label[192];		/* converted USB label for workaround */
    char cfg_path[128];			/* path to the ISO's isolinux.cfg */
    char reactos_path[128];		/* path to the ISO's freeldr.sys or setupldr.sys */
    char install_wim_path[64];	/* path to install.wim or install.swm */
    uint64_t projected_size;
    uint32_t install_wim_version;
    BOOLEAN is_iso;
    BOOLEAN is_bootable_img;
    uint8_t winpe;
    uint8_t has_efi;
    BOOLEAN has_4GB_file;
    BOOLEAN has_long_filename;
    BOOLEAN has_symlinks;
    BOOLEAN has_bootmgr;
    BOOLEAN has_autorun;
    BOOLEAN has_old_c32[NB_OLD_C32];
    BOOLEAN has_old_vesamenu;
    BOOLEAN has_efi_syslinux;
    BOOLEAN needs_syslinux_overwrite;
    BOOLEAN has_grub4dos;
    BOOLEAN has_grub2;
    BOOLEAN has_kolibrios;
    BOOLEAN uses_minint;
    BOOLEAN compression_type;
    BOOLEAN is_vhd;
    uint16_t sl_version;	// Syslinux/Isolinux version
    char sl_version_str[12];
    char sl_version_ext[32];
    char grub2_version[32];
} RUFUS_IMG_REPORT;


int recursive_iso_scan(uint32_t *loop_fds);

#endif // ISO_H
