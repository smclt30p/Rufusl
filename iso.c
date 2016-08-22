#define _XOPEN_SOURCE 500

#include <errno.h>
#include <ftw.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>

#include "iso.h"
#include "log.h"
#include "definitions.h"
#include "rufusl.h"

static char* dest = "/mnt/temp";
static iso_info_t info;

static const char* bootmgr_efi_name = "bootmgr.efi";
static const char* grldr_name = "grldr";
static const char* ldlinux_name = "ldlinux.sys";
static const char* ldlinux_c32 = "ldlinux.c32";

static const char* efi[] = { "efi/boot/bootia32.efi",
                             "efi/boot/bootia64.efi",
                             "efi/boot/bootx64.efi",
                             "efi/boot/bootarm.efi",
                             "efi/boot/bootaa64.efi",
                             "efi/boot/bootebc.efi" };

static const char* windows_wim[] = { "/sources/install.wim",
                                     "/sources/install.swm" };

static const char* grub_dirname = "/boot/grub/i386-pc";
static const char* grub_cfg = "grub.cfg";
static const char* syslinux_cfg[] = { "isolinux.cfg", "syslinux.cfg", "extlinux.conf" };
static const char* arch_cfg[] = { "archiso_sys32.cfg", "archiso_sys64.cfg" };
static const char* isolinux_tmp = ".\\isolinux.tmp";
static const char* isolinux_bin[] = { "isolinux.bin", "boot.bin" };
static const char* pe_dirname[] = { "/i386", "/minint" };
static const char* pe_file[] = { "ntdetect.com", "setupldr.bin", "txtsetup.sif" };
static const char* reactos_name = "setupldr.sys";
static const char* kolibri_name = "kolibri.img";
static const char* autorun_name = "autorun.inf";

int compare_string(const char *a, const char *b) {

    uint32_t len = strlen(a);

    if (len != strlen(b)) return -1;

    for (int i = 0; i < len; i++) {
        if (*(a+i) != *(b+i)) return -1;
    }

    return 0;

}

int iso_scan(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {

  /* Allocate new array and copy fpath into it because
     nftw() has an assertion if fpath has been changed. */

  char buf[strlen(fpath) + 1];
  memcpy(buf, fpath, strlen(fpath) + 1);

  /* Get length of path where the loop device is mounted */

  uint32_t a_len = strlen(TEMP_DIR_ISO);

  /* Trim soure path. Write each char from (buf + a_len + 1) to buf. */

  uint32_t i = a_len + 1;
  uint32_t j = 0;

  for (; j < sizeof(buf) - a_len; i++) {
    buf[j] = *(buf + i);
    j++;
  }

  buf[j] = 0x00; /* Null-terminate the string. Now we have a relative path */


  if (buf == NULL || strlen(buf) <= 0) return 0;

  if (sb->st_size > FAT32_MAX) {
      info.has_4gb = 1;
  }

  for(int i = 0; i < strlen(buf); i++){
    *(buf + i) = tolower(*(buf + i));
  }

  for (int i = 0; i < 5; i++) {
      if (compare_string(buf, efi[i]) == 0) {
          info.has_uefi = 1;
      }
  }

  return 0;
}

int recursive_iso_scan(uint32_t *loop_fd) {

    info.has_syslinux = 0;
    info.has_grub = 0;
    info.has_windows = 0;
    info.has_arch = 0;
    info.has_uefi = 0;
    info.has_autorun = 0;
    info.has_4gb = 0;

    if (lseek(*loop_fd, (off_t) LABEL_OFFSET, SEEK_SET) < 0) {
        r_printf("Falied to seek file: %s\n", strerror(errno));
        return -1;
    }

    memset(info.label, 0, 11);

    if (read(*loop_fd, info.label, (size_t) 11) < 0) {
        r_printf("Error reading label from ISO: %s\n", strerror(errno));
    }

    for(int i = 0; i < 11; i++){
      info.label[i] = toupper(info.label[i]);
    }

    r_printf(" * Label: %s\n", info.label);

    nftw(TEMP_DIR_ISO, iso_scan, 4, 0);

    if (info.has_uefi) {
        r_printf(" * Uses EFI\n");
    }

    if (lseek(*loop_fd, (off_t) 0, SEEK_SET) < 0) {
        r_printf("Falied to seek file: %s\n", strerror(errno));
        return -1;
    }

    set_ticker("READY");

}
