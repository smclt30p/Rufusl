/* mkfs.fat.c - utility to create FAT/MS-DOS filesystems

   Copyright (C) 1991 Linus Torvalds <torvalds@klaava.helsinki.fi>
   Copyright (C) 1992-1993 Remy Card <card@masi.ibp.fr>
   Copyright (C) 1993-1994 David Hudson <dave@humbug.demon.co.uk>
   Copyright (C) 1998 H. Peter Anvin <hpa@zytor.com>
   Copyright (C) 1998-2005 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
   Copyright (C) 2008-2014 Daniel Baumann <mail@daniel-baumann.ch>
   Copyright (C) 2015-2016 Andreas Bombe <aeb@debian.org>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.

   The complete text of the GNU General Public License
   can be found in /usr/share/common-licenses/GPL-3 file.
*/

/* Description: Utility to allow an MS-DOS filesystem to be created
   under Linux.  A lot of the basic structure of this program has been
   borrowed from Remy Card's "mke2fs" code.

   As far as possible the aim here is to make the "mkfs.fat" command
   look almost identical to the other Linux filesystem make utilties,
   eg bad blocks are still specified as blocks, not sectors, but when
   it comes down to it, DOS is tied to the idea of a sector (512 bytes
   as a rule), and not the block.  For example the boot block does not
   occupy a full cluster.

   Fixes/additions May 1998 by Roman Hodek
   <Roman.Hodek@informatik.uni-erlangen.de>:
   - Atari format support
   - New options -A, -S, -C
   - Support for filesystems > 2GB
   - FAT32 support */

/* Include the header files */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "device_info.h"
#include "msdos_fs.h"
#include "mkfs.fat.h"

#include "log.h"

/* Constant definitions */

#define TRUE 1 /* Boolean constants */
#define FALSE 0

#define TEST_BUFFER_BLOCKS 16
#define BLOCK_SIZE 1024
#define HARD_SECTOR_SIZE 512
#define SECTORS_PER_BLOCK (BLOCK_SIZE / HARD_SECTOR_SIZE)

#define NO_NAME "NO NAME    "

/* Macro definitions */

/* Report a failure message and return a failure error code */

#define die(str) fatal_error("%s: " str "\n")

/* Mark a cluster in the FAT as bad */

#define mark_sector_bad(sector) mark_FAT_sector(sector, FAT_BAD)

/* Compute ceil(a/b) */

static inline int cdiv(int a, int b) { return (a + b - 1) / b; }

/* FAT values */
#define FAT_EOF (atari_format ? 0x0fffffff : 0x0ffffff8)
#define FAT_BAD 0x0ffffff7

#define MSDOS_EXT_SIGN 0x29         /* extended boot sector signature */
#define MSDOS_FAT12_SIGN "FAT12   " /* FAT12 filesystem signature */
#define MSDOS_FAT16_SIGN "FAT16   " /* FAT16 filesystem signature */
#define MSDOS_FAT32_SIGN "FAT32   " /* FAT32 filesystem signature */

#define BOOT_SIGN 0xAA55 /* Boot sector magic number */

#define MAX_CLUST_12 ((1 << 12) - 16)
#define MAX_CLUST_16 ((1 << 16) - 16)
#define MIN_CLUST_32 65529
/* M$ says the high 4 bits of a FAT32 FAT entry are reserved and don't belong
 * to the cluster number. So the max. cluster# is based on 2^28 */
#define MAX_CLUST_32 ((1 << 28) - 16)

#define FAT12_THRESHOLD 4085

#define OLDGEMDOS_MAX_SECTORS 32765
#define GEMDOS_MAX_SECTORS 65531
#define GEMDOS_MAX_SECTOR_SIZE (16 * 1024)

#define BOOTCODE_SIZE 448
#define BOOTCODE_FAT32_SIZE 420

/* __attribute__ ((packed)) is used on all structures to make gcc ignore any
 * alignments */

struct msdos_volume_info {
  uint8_t drive_number;     /* BIOS drive number */
  uint8_t RESERVED;         /* Unused */
  uint8_t ext_boot_sign;    /* 0x29 if fields below exist (DOS 3.3+) */
  uint8_t volume_id[4];     /* Volume ID number */
  uint8_t volume_label[11]; /* Volume label */
  uint8_t fs_type[8];       /* Typically FAT12 or FAT16 */
} __attribute__((packed));

struct msdos_boot_sector {
  uint8_t boot_jump[3];   /* Boot strap short or near jump */
  uint8_t system_id[8];   /* Name - can be used to special case
                             partition manager volumes */
  uint8_t sector_size[2]; /* bytes per logical sector */
  uint8_t cluster_size;   /* sectors/cluster */
  uint16_t reserved;      /* reserved sectors */
  uint8_t fats;           /* number of FATs */
  uint8_t dir_entries[2]; /* root directory entries */
  uint8_t sectors[2];     /* number of sectors */
  uint8_t media;          /* media code (unused) */
  uint16_t fat_length;    /* sectors/FAT */
  uint16_t secs_track;    /* sectors per track */
  uint16_t heads;         /* number of heads */
  uint32_t hidden;        /* hidden sectors (unused) */
  uint32_t total_sect;    /* number of sectors (if sectors == 0) */
  union {
    struct {
      struct msdos_volume_info vi;
      uint8_t boot_code[BOOTCODE_SIZE];
    } __attribute__((packed)) _oldfat;
    struct {
      uint32_t fat32_length; /* sectors/FAT */
      uint16_t flags;        /* bit 8: fat mirroring, low 4: active fat */
      uint8_t version[2];    /* major, minor filesystem version */
      uint32_t root_cluster; /* first cluster in root directory */
      uint16_t info_sector;  /* filesystem info sector */
      uint16_t backup_boot;  /* backup boot sector */
      uint16_t reserved2[6]; /* Unused */
      struct msdos_volume_info vi;
      uint8_t boot_code[BOOTCODE_FAT32_SIZE];
    } __attribute__((packed)) _fat32;
  } __attribute__((packed)) fstype;
  uint16_t boot_sign;
} __attribute__((packed));
#define fat32 fstype._fat32
#define oldfat fstype._oldfat

struct fat32_fsinfo {
  uint32_t reserved1;     /* Nothing as far as I can tell */
  uint32_t signature;     /* 0x61417272L */
  uint32_t free_clusters; /* Free cluster count.  -1 if unknown */
  uint32_t next_cluster;  /* Most recently allocated cluster.
                           * Unused under Linux. */
  uint32_t reserved2[4];
};

/* The "boot code" we put into the filesystem... it writes a message and
   tells the user to try again */

unsigned char dummy_boot_jump[3] = {0xeb, 0x3c, 0x90};

unsigned char dummy_boot_jump_m68k[2] = {0x60, 0x1c};

#define MSG_OFFSET_OFFSET 3
char dummy_boot_code[BOOTCODE_SIZE] =
    "\x0e"         /* push cs */
    "\x1f"         /* pop ds */
    "\xbe\x5b\x7c" /* mov si, offset message_txt */
    /* write_msg: */
    "\xac"         /* lodsb */
    "\x22\xc0"     /* and al, al */
    "\x74\x0b"     /* jz key_press */
    "\x56"         /* push si */
    "\xb4\x0e"     /* mov ah, 0eh */
    "\xbb\x07\x00" /* mov bx, 0007h */
    "\xcd\x10"     /* int 10h */
    "\x5e"         /* pop si */
    "\xeb\xf0"     /* jmp write_msg */
    /* key_press: */
    "\x32\xe4" /* xor ah, ah */
    "\xcd\x16" /* int 16h */
    "\xcd\x19" /* int 19h */
    "\xeb\xfe" /* foo: jmp foo */
    /* message_txt: */
    "This is not a bootable disk.  Please insert a bootable floppy and\r\n"
    "press any key to try again ... \r\n";

#define MESSAGE_OFFSET 29 /* Offset of message in above code */

/* Global variables - the root of all evil :-) - see these and weep! */

static const char *program_name = "mkfs.fat"; /* Name of the program */
static char *device_name =
    NULL; /* Name of the device on which to create the filesystem */
static int atari_format = 0; /* Use Atari variation of MS-DOS FS format */
static int check = FALSE;    /* Default to no readablity checking */
static int verbose = 0;      /* Default to verbose mode off */
static long volume_id;       /* Volume ID number */
static time_t create_time;   /* Creation time */
static char volume_name[] = NO_NAME; /* Volume name */
static uint64_t blocks;              /* Number of blocks in filesystem */
static int sector_size = 512;        /* Size of a logical sector */
static int sector_size_set = 0;      /* User selected sector size */
static int backup_boot = 0;          /* Sector# of backup boot sector */
static int reserved_sectors = 0;     /* Number of reserved sectors */
static int badblocks = 0;        /* Number of bad blocks in the filesystem */
static int nr_fats = 2;          /* Default number of FATs to produce */
static int size_fat = 0;         /* Size in bits of FAT entries */
static int size_fat_by_user = 0; /* 1 if FAT size user selected */
static int dev = -1;             /* FS block device file handle */
static off_t currently_testing =
    0; /* Block currently being tested (if autodetect bad blocks) */
static struct msdos_boot_sector bs; /* Boot sector data */
static int start_data_sector; /* Sector number for the start of the data area */
static int start_data_block;  /* Block number for the start of the data area */
static unsigned char *fat;    /* File allocation table */
static unsigned alloced_fat_length; /* # of FAT sectors we can keep in memory */
static unsigned
    fat_entries; /* total entries in FAT table (including reserved) */
static unsigned char *info_sector;       /* FAT32 info sector */
static struct msdos_dir_entry *root_dir; /* Root directory */
static int size_root_dir;              /* Size of the root directory in bytes */
static uint32_t num_sectors;           /* Total number of sectors in device */
static int sectors_per_cluster = 0;    /* Number of sectors per disk cluster */
static int root_dir_entries = 0;       /* Number of root directory entries */
static char *blank_sector;             /* Blank sector - all zeros */
static int hidden_sectors = 0;         /* Number of hidden sectors */
static int hidden_sectors_by_user = 0; /* -h option invoked */
static int drive_number_option = 0;    /* drive number */
static int drive_number_by_user = 0;   /* drive number option invoked */
static int fat_media_byte = 0; /* media byte in header and starting FAT */
static int malloc_entire_fat =
    FALSE; /* Whether we should malloc() the entire FAT or not */
static int align_structures = TRUE; /* Whether to enforce alignment */
static int orphaned_sectors =
    0; /* Sectors that exist in the last block of filesystem */
static int invariant = 0; /* Whether to set normally randomized or
                             current time based values to
                             constants */

/* Function prototype definitions */

static void fatal_error( char *fmt_string);
static void mark_FAT_cluster(int cluster, unsigned int value);
static void mark_FAT_sector(int sector, unsigned int value);
static long do_check(char *buffer, int try, off_t current_block);
static void alarm_intr(int alnum);
static void check_blocks(void);
static void establish_params(struct device_info *info);
static void setup_tables(void);
static void write_tables(void);

/* The function implementations */

/* Handle the reporting of fatal errors.  Volatile to let gcc know that this
 * doesn't return */

static void fatal_error(char *fmt_string) {
  r_printf(fmt_string, program_name, device_name);
}

/* Mark the specified cluster as having a particular value */

static void mark_FAT_cluster(int cluster, unsigned int value) {
  if (cluster < 0 || cluster >= fat_entries)
    die("Internal error: out of range cluster number in mark_FAT_cluster");

  switch (size_fat) {
    case 12:
      value &= 0x0fff;
      if (((cluster * 3) & 0x1) == 0) {
        fat[3 * cluster / 2] = (unsigned char)(value & 0x00ff);
        fat[(3 * cluster / 2) + 1] =
            (unsigned char)((fat[(3 * cluster / 2) + 1] & 0x00f0) |
                            ((value & 0x0f00) >> 8));
      } else {
        fat[3 * cluster / 2] = (unsigned char)((fat[3 * cluster / 2] & 0x000f) |
                                               ((value & 0x000f) << 4));
        fat[(3 * cluster / 2) + 1] = (unsigned char)((value & 0x0ff0) >> 4);
      }
      break;

    case 16:
      value &= 0xffff;
      fat[2 * cluster] = (unsigned char)(value & 0x00ff);
      fat[(2 * cluster) + 1] = (unsigned char)(value >> 8);
      break;

    case 32:
      value &= 0xfffffff;
      fat[4 * cluster] = (unsigned char)(value & 0x000000ff);
      fat[(4 * cluster) + 1] = (unsigned char)((value & 0x0000ff00) >> 8);
      fat[(4 * cluster) + 2] = (unsigned char)((value & 0x00ff0000) >> 16);
      fat[(4 * cluster) + 3] = (unsigned char)((value & 0xff000000) >> 24);
      break;

    default:
      die("Bad FAT size (not 12, 16, or 32)");
  }
}

/* Mark a specified sector as having a particular value in it's FAT entry */

static void mark_FAT_sector(int sector, unsigned int value) {
  int cluster = (sector - start_data_sector) / (int)(bs.cluster_size) /
                    (sector_size / HARD_SECTOR_SIZE) +
                2;

  if (sector < start_data_sector || sector >= num_sectors)
    die("Internal error: out of range sector number in mark_FAT_sector");

  mark_FAT_cluster(cluster, value);
}

/* Perform a test on a block.  Return the number of blocks that could be read
 * successfully */

static long do_check(char *buffer, int try, off_t current_block) {
  long got;

  if (lseek(dev, current_block * BLOCK_SIZE,
            SEEK_SET) /* Seek to the correct location */
      != current_block * BLOCK_SIZE)
    die("seek failed during testing for blocks");

  got = read(dev, buffer, try * BLOCK_SIZE); /* Try reading! */
  if (got < 0) got = 0;

  if (got & (BLOCK_SIZE - 1))
    r_printf("Unexpected values in do_check: probably bugs\n");
  got /= BLOCK_SIZE;

  return got;
}

/* Alarm clock handler - display the status of the quest for bad blocks!  Then
   retrigger the alarm for five senconds
   later (so we can come here again) */

static void alarm_intr(int alnum) {
  (void)alnum;

  if (currently_testing >= blocks) return;

  signal(SIGALRM, alarm_intr);
  alarm(5);
  if (!currently_testing) return;

  r_printf("%lld... ", (unsigned long long)currently_testing);
  fflush(stdout);
}

static void check_blocks(void) {
  int try
    , got;
  int i;
  static char blkbuf[BLOCK_SIZE * TEST_BUFFER_BLOCKS];

  if (verbose) {
    r_printf("Searching for bad blocks ");
    fflush(stdout);
  }
  currently_testing = 0;
  if (verbose) {
    signal(SIGALRM, alarm_intr);
    alarm(5);
  }
  try
    = TEST_BUFFER_BLOCKS;
  while (currently_testing < blocks) {
    if (currently_testing + try > blocks) try
        = blocks - currently_testing;
    got = do_check(blkbuf, try, currently_testing);
    currently_testing += got;
    if (got == try) {
      try
        = TEST_BUFFER_BLOCKS;
      continue;
    } else
      try
        = 1;
    if (currently_testing < start_data_block)
      die("bad blocks before data-area: cannot make fs");

    for (i = 0; i < SECTORS_PER_BLOCK;
         i++) /* Mark all of the sectors in the block as bad */
      mark_sector_bad(currently_testing * SECTORS_PER_BLOCK + i);
    badblocks++;
    currently_testing++;
  }

  if (verbose) r_printf("\n");

  if (badblocks)
    r_printf("%d bad block%s\n", badblocks, (badblocks > 1) ? "s" : "");
}

/* Establish the geometry and media parameters for the device */

static void establish_params(struct device_info *info) {
  unsigned int sec_per_track = 63;
  unsigned int heads = 255;
  unsigned int media = 0xf8;
  unsigned int cluster_size = 4; /* starting point for FAT12 and FAT16 */
  int def_root_dir_entries = 512;

  if (info->type != TYPE_FIXED) {
    /* enter default parameters for floppy disks if the size matches */
    switch (info->size / 1024) {
      case 360:
        sec_per_track = 9;
        heads = 2;
        media = 0xfd;
        cluster_size = 2;
        def_root_dir_entries = 112;
        break;

      case 720:
        sec_per_track = 9;
        heads = 2;
        media = 0xf9;
        cluster_size = 2;
        def_root_dir_entries = 112;
        break;

      case 1200:
        sec_per_track = 15;
        heads = 2;
        media = 0xf9;
        cluster_size = (atari_format ? 2 : 1);
        def_root_dir_entries = 224;
        break;

      case 1440:
        sec_per_track = 18;
        heads = 2;
        media = 0xf0;
        cluster_size = (atari_format ? 2 : 1);
        def_root_dir_entries = 224;
        break;

      case 2880:
        sec_per_track = 36;
        heads = 2;
        media = 0xf0;
        cluster_size = 2;
        def_root_dir_entries = 224;
        break;
    }
  }

  if (!size_fat && info->size >= 512 * 1024 * 1024) {
    if (verbose) r_printf("Auto-selecting FAT32 for large filesystem\n");
    size_fat = 32;
  }
  if (size_fat == 32) {
    /*
     * For FAT32, try to do the same as M$'s format command
     * (see http://www.win.tue.nl/~aeb/linux/fs/fat/fatgen103.pdf p. 20):
     * fs size <= 260M: 0.5k clusters
     * fs size <=   8G:   4k clusters
     * fs size <=  16G:   8k clusters
     * fs size <=  32G:  16k clusters
     * fs size >   32G:  32k clusters
     *
     * This only works correctly for 512 byte sectors!
     */
    uint32_t sz_mb = info->size / (1024 * 1024);
    cluster_size = sz_mb > 32 * 1024 ? 64 : sz_mb > 16 * 1024
                                                ? 32
                                                : sz_mb > 8 * 1024
                                                      ? 16
                                                      : sz_mb > 260 ? 8 : 1;
  }

  if (info->geom_heads > 0) {
    heads = info->geom_heads;
    sec_per_track = info->geom_sectors;
  }

  if (!hidden_sectors_by_user && info->geom_start >= 0)
    hidden_sectors = htole32(info->geom_start);

  if (!root_dir_entries) root_dir_entries = def_root_dir_entries;

  bs.secs_track = htole16(sec_per_track);
  bs.heads = htole16(heads);
  bs.media = media;
  bs.cluster_size = cluster_size;
}

/*
 * If alignment is enabled, round the first argument up to the second; the
 * latter must be a power of two.
 */
static unsigned int align_object(unsigned int sectors, unsigned int clustsize) {
  if (align_structures)
    return (sectors + clustsize - 1) & ~(clustsize - 1);
  else
    return sectors;
}

/* Create the filesystem data tables */

static void setup_tables(void) {
  unsigned cluster_count = 0, fat_length;
  struct tm *ctime;
  struct msdos_volume_info *vi =
      (size_fat == 32 ? &bs.fat32.vi : &bs.oldfat.vi);

  if (atari_format) {
    /* On Atari, the first few bytes of the boot sector are assigned
     * differently: The jump code is only 2 bytes (and m68k machine code
     * :-), then 6 bytes filler (ignored), then 3 byte serial number. */
    bs.boot_jump[2] = 'm';
    memcpy((char *)bs.system_id, "kdosf", strlen("kdosf"));
  } else
    memcpy((char *)bs.system_id, "mkfs.fat", strlen("mkfs.fat"));
  if (sectors_per_cluster) bs.cluster_size = (char)sectors_per_cluster;

  if (fat_media_byte) bs.media = (char)fat_media_byte;

  if (bs.media == 0xf8)
    vi->drive_number = 0x80;
  else
    vi->drive_number = 0x00;

  if (drive_number_by_user) vi->drive_number = (char)drive_number_option;

  if (size_fat == 32) {
    /* Under FAT32, the root dir is in a cluster chain, and this is
     * signalled by bs.dir_entries being 0. */
    root_dir_entries = 0;
  }

  if (atari_format) {
    bs.system_id[5] = (unsigned char)(volume_id & 0x000000ff);
    bs.system_id[6] = (unsigned char)((volume_id & 0x0000ff00) >> 8);
    bs.system_id[7] = (unsigned char)((volume_id & 0x00ff0000) >> 16);
  } else {
    vi->volume_id[0] = (unsigned char)(volume_id & 0x000000ff);
    vi->volume_id[1] = (unsigned char)((volume_id & 0x0000ff00) >> 8);
    vi->volume_id[2] = (unsigned char)((volume_id & 0x00ff0000) >> 16);
    vi->volume_id[3] = (unsigned char)(volume_id >> 24);
  }

  if (!atari_format) {
    memcpy(vi->volume_label, volume_name, 11);

    memcpy(bs.boot_jump, dummy_boot_jump, 3);
    /* Patch in the correct offset to the boot code */
    bs.boot_jump[1] = ((size_fat == 32 ? (char *)&bs.fat32.boot_code
                                       : (char *)&bs.oldfat.boot_code) -
                       (char *)&bs) -
                      2;

    if (size_fat == 32) {
      int offset =
          (char *)&bs.fat32.boot_code - (char *)&bs + MESSAGE_OFFSET + 0x7c00;
      if (dummy_boot_code[BOOTCODE_FAT32_SIZE - 1])
        r_printf("Warning: message too long; truncated\n");
      dummy_boot_code[BOOTCODE_FAT32_SIZE - 1] = 0;
      memcpy(bs.fat32.boot_code, dummy_boot_code, BOOTCODE_FAT32_SIZE);
      bs.fat32.boot_code[MSG_OFFSET_OFFSET] = offset & 0xff;
      bs.fat32.boot_code[MSG_OFFSET_OFFSET + 1] = offset >> 8;
    } else {
      memcpy(bs.oldfat.boot_code, dummy_boot_code, BOOTCODE_SIZE);
    }
    bs.boot_sign = htole16(BOOT_SIGN);
  } else {
    memcpy(bs.boot_jump, dummy_boot_jump_m68k, 2);
  }
  if (verbose >= 2)
    r_printf("Boot jump code is %02x %02x\n", bs.boot_jump[0], bs.boot_jump[1]);

  if (!reserved_sectors)
    reserved_sectors = (size_fat == 32) ? 32 : 1;
  else {
    if (size_fat == 32 && reserved_sectors < 2)
      die("On FAT32 at least 2 reserved sectors are needed.");
  }
  bs.reserved = htole16(reserved_sectors);
  if (verbose >= 2) r_printf("Using %d reserved sectors\n", reserved_sectors);
  bs.fats = (char)nr_fats;
  if (!atari_format || size_fat == 32)
    bs.hidden = htole32(hidden_sectors);
  else {
    /* In Atari format, hidden is a 16 bit field */
    uint16_t hidden = htole16(hidden_sectors);
    if (hidden_sectors & ~0xffff)
      die("#hidden doesn't fit in 16bit field of Atari format\n");
    memcpy(&bs.hidden, &hidden, 2);
  }

  if ((long long)(blocks * BLOCK_SIZE / sector_size) + orphaned_sectors >
      UINT32_MAX) {
    r_printf("Warning: target too large, space at end will be left unused\n");
    num_sectors = UINT32_MAX;
    blocks = (uint64_t)UINT32_MAX * sector_size / BLOCK_SIZE;
  } else {
    num_sectors =
        (long long)(blocks * BLOCK_SIZE / sector_size) + orphaned_sectors;
  }

  if (!atari_format) {
    unsigned fatdata1216; /* Sectors for FATs + data area (FAT12/16) */
    unsigned fatdata32;   /* Sectors for FATs + data area (FAT32) */
    unsigned fatlength12, fatlength16, fatlength32;
    unsigned maxclust12, maxclust16, maxclust32;
    unsigned clust12, clust16, clust32;
    int maxclustsize;
    unsigned root_dir_sectors = cdiv(root_dir_entries * 32, sector_size);

    /*
     * If the filesystem is 8192 sectors or less (4 MB with 512-byte
     * sectors, i.e. floppy size), don't align the data structures.
     */
    if (num_sectors <= 8192) {
      if (align_structures && verbose >= 2)
        r_printf("Disabling alignment due to tiny filesystem\n");

      align_structures = FALSE;
    }

    if (sectors_per_cluster)
      bs.cluster_size = maxclustsize = sectors_per_cluster;
    else
      /* An initial guess for bs.cluster_size should already be set */
      maxclustsize = 128;

    do {
      fatdata32 = num_sectors - align_object(reserved_sectors, bs.cluster_size);
      fatdata1216 = fatdata32 - align_object(root_dir_sectors, bs.cluster_size);

      if (verbose >= 2)
        r_printf("Trying with %d sectors/cluster:\n", bs.cluster_size);

      /* The factor 2 below avoids cut-off errors for nr_fats == 1.
       * The "nr_fats*3" is for the reserved first two FAT entries */
      clust12 = 2 * ((long long)fatdata1216 * sector_size + nr_fats * 3) /
                (2 * (int)bs.cluster_size * sector_size + nr_fats * 3);
      fatlength12 = cdiv(((clust12 + 2) * 3 + 1) >> 1, sector_size);
      fatlength12 = align_object(fatlength12, bs.cluster_size);
      /* Need to recalculate number of clusters, since the unused parts of the
       * FATS and data area together could make up space for an additional,
       * not really present cluster. */
      clust12 = (fatdata1216 - nr_fats * fatlength12) / bs.cluster_size;
      maxclust12 = (fatlength12 * 2 * sector_size) / 3;
      if (maxclust12 > MAX_CLUST_12) maxclust12 = MAX_CLUST_12;
      if (verbose >= 2)
        r_printf("FAT12: #clu=%u, fatlen=%u, maxclu=%u, limit=%u\n", clust12,
               fatlength12, maxclust12, MAX_CLUST_12);
      if (clust12 > maxclust12 - 2) {
        clust12 = 0;
        if (verbose >= 2) r_printf("FAT12: too much clusters\n");
      }

      clust16 = ((long long)fatdata1216 * sector_size + nr_fats * 4) /
                ((int)bs.cluster_size * sector_size + nr_fats * 2);
      fatlength16 = cdiv((clust16 + 2) * 2, sector_size);
      fatlength16 = align_object(fatlength16, bs.cluster_size);
      /* Need to recalculate number of clusters, since the unused parts of the
       * FATS and data area together could make up space for an additional,
       * not really present cluster. */
      clust16 = (fatdata1216 - nr_fats * fatlength16) / bs.cluster_size;
      maxclust16 = (fatlength16 * sector_size) / 2;
      if (maxclust16 > MAX_CLUST_16) maxclust16 = MAX_CLUST_16;
      if (verbose >= 2)
        r_printf("FAT16: #clu=%u, fatlen=%u, maxclu=%u, limit=%u\n", clust16,
               fatlength16, maxclust16, MAX_CLUST_16);
      if (clust16 > maxclust16 - 2) {
        if (verbose >= 2) r_printf("FAT16: too much clusters\n");
        clust16 = 0;
      }
      /* The < 4078 avoids that the filesystem will be misdetected as having a
       * 12 bit FAT. */
      if (clust16 < FAT12_THRESHOLD && !(size_fat_by_user && size_fat == 16)) {
        if (verbose >= 2)
          r_printf(clust16 < FAT12_THRESHOLD
                     ? "FAT16: would be misdetected as FAT12\n"
                     : "FAT16: too much clusters\n");
        clust16 = 0;
      }

      clust32 = ((long long)fatdata32 * sector_size + nr_fats * 8) /
                ((int)bs.cluster_size * sector_size + nr_fats * 4);
      fatlength32 = cdiv((clust32 + 2) * 4, sector_size);
      fatlength32 = align_object(fatlength32, bs.cluster_size);
      /* Need to recalculate number of clusters, since the unused parts of the
       * FATS and data area together could make up space for an additional,
       * not really present cluster. */
      clust32 = (fatdata32 - nr_fats * fatlength32) / bs.cluster_size;
      maxclust32 = (fatlength32 * sector_size) / 4;
      if (maxclust32 > MAX_CLUST_32) maxclust32 = MAX_CLUST_32;
      if (clust32 && clust32 < MIN_CLUST_32 &&
          !(size_fat_by_user && size_fat == 32)) {
        clust32 = 0;
        if (verbose >= 2)
          r_printf("FAT32: not enough clusters (%d)\n", MIN_CLUST_32);
      }
      if (verbose >= 2)
        r_printf("FAT32: #clu=%u, fatlen=%u, maxclu=%u, limit=%u\n", clust32,
               fatlength32, maxclust32, MAX_CLUST_32);
      if (clust32 > maxclust32) {
        clust32 = 0;
        if (verbose >= 2) r_printf("FAT32: too much clusters\n");
      }

      if ((clust12 && (size_fat == 0 || size_fat == 12)) ||
          (clust16 && (size_fat == 0 || size_fat == 16)) ||
          (clust32 && size_fat == 32))
        break;

      bs.cluster_size <<= 1;
    } while (bs.cluster_size && bs.cluster_size <= maxclustsize);

    /* Use the optimal FAT size if not specified;
     * FAT32 is (not yet) choosen automatically */
    if (!size_fat) {
      size_fat = (clust16 > clust12) ? 16 : 12;
      if (verbose >= 2) r_printf("Choosing %d bits for FAT\n", size_fat);
    }

    switch (size_fat) {
      case 12:
        cluster_count = clust12;
        fat_length = fatlength12;
        bs.fat_length = htole16(fatlength12);
        memcpy(vi->fs_type, MSDOS_FAT12_SIGN, 8);
        break;

      case 16:
        if (clust16 < FAT12_THRESHOLD) {
          if (size_fat_by_user) {
            r_printf("WARNING: Not enough clusters for a "
                    "16 bit FAT! The filesystem will be\n"
                    "misinterpreted as having a 12 bit FAT without "
                    "mount option \"fat=16\".\n");
          } else {
            r_printf("This filesystem has an unfortunate size. "
                    "A 12 bit FAT cannot provide\n"
                    "enough clusters, but a 16 bit FAT takes up a little "
                    "bit more space so that\n"
                    "the total number of clusters becomes less than the "
                    "threshold value for\n"
                    "distinction between 12 and 16 bit FATs.\n");
            die("Make the filesystem a bit smaller manually.");
          }
        }
        cluster_count = clust16;
        fat_length = fatlength16;
        bs.fat_length = htole16(fatlength16);
        memcpy(vi->fs_type, MSDOS_FAT16_SIGN, 8);
        break;

      case 32:
        if (clust32 < MIN_CLUST_32)
          r_printf("WARNING: Not enough clusters for a 32 bit FAT!\n");
        cluster_count = clust32;
        fat_length = fatlength32;
        bs.fat_length = htole16(0);
        bs.fat32.fat32_length = htole32(fatlength32);
        memcpy(vi->fs_type, MSDOS_FAT32_SIGN, 8);
        root_dir_entries = 0;
        break;

      default:
        die("FAT not 12, 16 or 32 bits");
    }

    /* Adjust the reserved number of sectors for alignment */
    reserved_sectors = align_object(reserved_sectors, bs.cluster_size);
    bs.reserved = htole16(reserved_sectors);

    /* Adjust the number of root directory entries to help enforce alignment */
    if (align_structures) {
      root_dir_entries =
          align_object(root_dir_sectors, bs.cluster_size) * (sector_size >> 5);
    }
  } else {
    unsigned clusters, maxclust, fatdata;

    /* GEMDOS always uses a 12 bit FAT on floppies, and always a 16 bit FAT on
     * hard disks. So use 12 bit if the size of the filesystem suggests that
     * this fs is for a floppy disk, if the user hasn't explicitly requested a
     * size.
     */
    if (!size_fat)
      size_fat = (num_sectors == 1440 || num_sectors == 2400 ||
                  num_sectors == 2880 || num_sectors == 5760)
                     ? 12
                     : 16;
    if (verbose >= 2) r_printf("Choosing %d bits for FAT\n", size_fat);

    /* Atari format: cluster size should be 2, except explicitly requested by
     * the user, since GEMDOS doesn't like other cluster sizes very much.
     * Instead, tune the sector size for the FS to fit.
     */
    bs.cluster_size = sectors_per_cluster ? sectors_per_cluster : 2;
    if (!sector_size_set) {
      while (num_sectors > GEMDOS_MAX_SECTORS) {
        num_sectors >>= 1;
        sector_size <<= 1;
      }
    }
    if (verbose >= 2)
      r_printf("Sector size must be %d to have less than %d log. sectors\n",
             sector_size, GEMDOS_MAX_SECTORS);

    /* Check if there are enough FAT indices for how much clusters we have */
    do {
      fatdata = num_sectors - cdiv(root_dir_entries * 32, sector_size) -
                reserved_sectors;
      /* The factor 2 below avoids cut-off errors for nr_fats == 1 and
       * size_fat == 12
       * The "2*nr_fats*size_fat/8" is for the reserved first two FAT entries
       */
      clusters =
          (2 *
           ((long long)fatdata * sector_size - 2 * nr_fats * size_fat / 8)) /
          (2 * ((int)bs.cluster_size * sector_size + nr_fats * size_fat / 8));
      fat_length = cdiv((clusters + 2) * size_fat / 8, sector_size);
      /* Need to recalculate number of clusters, since the unused parts of the
       * FATS and data area together could make up space for an additional,
       * not really present cluster. */
      clusters = (fatdata - nr_fats * fat_length) / bs.cluster_size;
      maxclust = (fat_length * sector_size * 8) / size_fat;
      if (verbose >= 2)
        r_printf("ss=%d: #clu=%d, fat_len=%d, maxclu=%d\n", sector_size, clusters,
               fat_length, maxclust);

      /* last 10 cluster numbers are special (except FAT32: 4 high bits rsvd);
       * first two numbers are reserved */
      if (maxclust <=
              (size_fat == 32 ? MAX_CLUST_32 : (1 << size_fat) - 0x10) &&
          clusters <= maxclust - 2)
        break;
      if (verbose >= 2)
        r_printf(clusters > maxclust - 2 ? "Too many clusters\n"
                                       : "FAT too big\n");

      /* need to increment sector_size once more to  */
      if (sector_size_set)
        die("With this sector size, the maximum number of FAT entries "
            "would be exceeded.");
      num_sectors >>= 1;
      sector_size <<= 1;
    } while (sector_size <= GEMDOS_MAX_SECTOR_SIZE);

    if (sector_size > GEMDOS_MAX_SECTOR_SIZE)
      die("Would need a sector size > 16k, which GEMDOS can't work with");

    cluster_count = clusters;
    if (size_fat != 32)
      bs.fat_length = htole16(fat_length);
    else {
      bs.fat_length = 0;
      bs.fat32.fat32_length = htole32(fat_length);
    }
  }

  bs.sector_size[0] = (char)(sector_size & 0x00ff);
  bs.sector_size[1] = (char)((sector_size & 0xff00) >> 8);

  bs.dir_entries[0] = (char)(root_dir_entries & 0x00ff);
  bs.dir_entries[1] = (char)((root_dir_entries & 0xff00) >> 8);

  if (size_fat == 32) {
    /* set up additional FAT32 fields */
    bs.fat32.flags = htole16(0);
    bs.fat32.version[0] = 0;
    bs.fat32.version[1] = 0;
    bs.fat32.root_cluster = htole32(2);
    bs.fat32.info_sector = htole16(1);
    if (!backup_boot)
      backup_boot = (reserved_sectors >= 7) ? 6 : (reserved_sectors >= 2)
                                                      ? reserved_sectors - 1
                                                      : 0;
    else {
      if (backup_boot == 1)
        die("Backup boot sector must be after sector 1");
      else if (backup_boot >= reserved_sectors)
        die("Backup boot sector must be a reserved sector");
    }
    if (verbose >= 2)
      r_printf("Using sector %d as backup boot sector (0 = none)\n", backup_boot);
    bs.fat32.backup_boot = htole16(backup_boot);
    memset(&bs.fat32.reserved2, 0, sizeof(bs.fat32.reserved2));
  }

  if (atari_format) {
    /* Just some consistency checks */
    if (num_sectors >= GEMDOS_MAX_SECTORS)
      die("GEMDOS can't handle more than 65531 sectors");
    else if (num_sectors >= OLDGEMDOS_MAX_SECTORS)
      r_printf("Warning: More than 32765 sector need TOS 1.04 "
          "or higher.\n");
  }
  if (num_sectors >= 65536) {
    bs.sectors[0] = (char)0;
    bs.sectors[1] = (char)0;
    bs.total_sect = htole32(num_sectors);
  } else {
    bs.sectors[0] = (char)(num_sectors & 0x00ff);
    bs.sectors[1] = (char)((num_sectors & 0xff00) >> 8);
    if (!atari_format) bs.total_sect = htole32(0);
  }

  if (!atari_format) vi->ext_boot_sign = MSDOS_EXT_SIGN;

  if (!cluster_count) {
    if (sectors_per_cluster) /* If yes, die if we'd spec'd sectors per cluster
                                */
      die("Too many clusters for filesystem - try more sectors per cluster");
    else
      die("Attempting to create a too large filesystem");
  }
  fat_entries = cluster_count + 2;

  /* The two following vars are in hard sectors, i.e. 512 byte sectors! */
  start_data_sector = (reserved_sectors + nr_fats * fat_length +
                       cdiv(root_dir_entries * 32, sector_size)) *
                      (sector_size / HARD_SECTOR_SIZE);
  start_data_block =
      (start_data_sector + SECTORS_PER_BLOCK - 1) / SECTORS_PER_BLOCK;

  if (blocks < start_data_block + 32) /* Arbitrary undersize filesystem! */
    die("Too few blocks for viable filesystem");

  if (verbose) {
    r_printf("%s has %d head%s and %d sector%s per track,\n", device_name,
           le16toh(bs.heads), (le16toh(bs.heads) != 1) ? "s" : "",
           le16toh(bs.secs_track), (le16toh(bs.secs_track) != 1) ? "s" : "");
    r_printf("hidden sectors 0x%04x;\n", hidden_sectors);
    r_printf("logical sector size is %d,\n", sector_size);
    r_printf("using 0x%02x media descriptor, with %d sectors;\n", (int)(bs.media),
           num_sectors);
    r_printf("drive number 0x%02x;\n", (int)(vi->drive_number));
    r_printf("filesystem has %d %d-bit FAT%s and %d sector%s per cluster.\n",
           (int)(bs.fats), size_fat, (bs.fats != 1) ? "s" : "",
           (int)(bs.cluster_size), (bs.cluster_size != 1) ? "s" : "");
    r_printf("FAT size is %d sector%s, and provides %d cluster%s.\n", fat_length,
           (fat_length != 1) ? "s" : "", cluster_count,
           (cluster_count != 1) ? "s" : "");
    r_printf("There %s %u reserved sector%s.\n",
           (reserved_sectors != 1) ? "are" : "is", reserved_sectors,
           (reserved_sectors != 1) ? "s" : "");

    if (size_fat != 32) {
      unsigned root_dir_entries =
          bs.dir_entries[0] + ((bs.dir_entries[1]) * 256);
      unsigned root_dir_sectors = cdiv(root_dir_entries * 32, sector_size);
      r_printf("Root directory contains %u slots and uses %u sectors.\n",
             root_dir_entries, root_dir_sectors);
    }
    r_printf("Volume ID is %08lx, ",
           volume_id & (atari_format ? 0x00ffffff : 0xffffffff));
    if (strcmp(volume_name, NO_NAME))
      r_printf("volume label %s.\n", volume_name);
    else
      r_printf("no volume label.\n");
  }

  /* Make the file allocation tables! */

  if (malloc_entire_fat)
    alloced_fat_length = fat_length;
  else
    alloced_fat_length = 1;

  if ((fat = (unsigned char *)malloc(alloced_fat_length * sector_size)) == NULL)
    die("unable to allocate space for FAT image in memory");

  memset(fat, 0, alloced_fat_length * sector_size);

  mark_FAT_cluster(0, 0xffffffff); /* Initial fat entries */
  mark_FAT_cluster(1, 0xffffffff);
  fat[0] = (unsigned char)bs.media; /* Put media type in first byte! */
  if (size_fat == 32) {
    /* Mark cluster 2 as EOF (used for root dir) */
    mark_FAT_cluster(2, FAT_EOF);
  }

  /* Make the root directory entries */

  size_root_dir =
      (size_fat == 32)
          ? bs.cluster_size * sector_size
          : (((int)bs.dir_entries[1] * 256 + (int)bs.dir_entries[0]) *
             sizeof(struct msdos_dir_entry));
  if ((root_dir = (struct msdos_dir_entry *)malloc(size_root_dir)) == NULL) {
    free(fat); /* Tidy up before we die! */
    die("unable to allocate space for root directory in memory");
  }

  memset(root_dir, 0, size_root_dir);
  if (memcmp(volume_name, NO_NAME, MSDOS_NAME)) {
    struct msdos_dir_entry *de = &root_dir[0];
    memcpy(de->name, volume_name, MSDOS_NAME);
    de->attr = ATTR_VOLUME;
    if (!invariant)
      ctime = localtime(&create_time);
    else
      ctime = gmtime(&create_time);
    de->time =
        htole16((unsigned short)((ctime->tm_sec >> 1) + (ctime->tm_min << 5) +
                                 (ctime->tm_hour << 11)));
    de->date =
        htole16((unsigned short)(ctime->tm_mday + ((ctime->tm_mon + 1) << 5) +
                                 ((ctime->tm_year - 80) << 9)));
    de->ctime_cs = 0;
    de->ctime = de->time;
    de->cdate = de->date;
    de->adate = de->date;
    de->starthi = htole16(0);
    de->start = htole16(0);
    de->size = htole32(0);
  }

  if (size_fat == 32) {
    /* For FAT32, create an info sector */
    struct fat32_fsinfo *info;

    if (!(info_sector = malloc(sector_size))) die("Out of memory");
    memset(info_sector, 0, sector_size);
    /* fsinfo structure is at offset 0x1e0 in info sector by observation */
    info = (struct fat32_fsinfo *)(info_sector + 0x1e0);

    /* Info sector magic */
    info_sector[0] = 'R';
    info_sector[1] = 'R';
    info_sector[2] = 'a';
    info_sector[3] = 'A';

    /* Magic for fsinfo structure */
    info->signature = htole32(0x61417272);
    /* We've allocated cluster 2 for the root dir. */
    info->free_clusters = htole32(cluster_count - 1);
    info->next_cluster = htole32(2);

    /* Info sector also must have boot sign */
    *(uint16_t *)(info_sector + 0x1fe) = htole16(BOOT_SIGN);
  }

  if (!(blank_sector = malloc(sector_size))) die("Out of memory");
  memset(blank_sector, 0, sector_size);
}

/* Write the new filesystem's data tables to wherever they're going to end up!
 */

#define error(str)                      \
  do {                                  \
    free(fat);                          \
    if (info_sector) free(info_sector); \
    free(root_dir);                     \
    die(str);                           \
  } while (0)

#define seekto(pos, errstr)                                     \
  do {                                                          \
    off_t __pos = (pos);                                        \
    if (lseek(dev, __pos, SEEK_SET) != __pos)                   \
      error("seek to " errstr " failed whilst writing tables"); \
  } while (0)

#define writebuf(buf, size, errstr)           \
  do {                                        \
    int __size = (size);                      \
    if (write(dev, buf, __size) != __size)    \
      error("failed whilst writing " errstr); \
  } while (0)

static void write_tables(void) {
  int x;
  int fat_length;

  fat_length = (size_fat == 32) ? le32toh(bs.fat32.fat32_length)
                                : le16toh(bs.fat_length);

  seekto(0, "start of device");
  /* clear all reserved sectors */
  for (x = 0; x < reserved_sectors; ++x)
    writebuf(blank_sector, sector_size, "reserved sector");
  /* seek back to sector 0 and write the boot sector */
  seekto(0, "boot sector");
  writebuf((char *)&bs, sizeof(struct msdos_boot_sector), "boot sector");
  /* on FAT32, write the info sector and backup boot sector */
  if (size_fat == 32) {
    seekto(le16toh(bs.fat32.info_sector) * sector_size, "info sector");
    writebuf(info_sector, 512, "info sector");
    if (backup_boot != 0) {
      seekto(backup_boot * sector_size, "backup boot sector");
      writebuf((char *)&bs, sizeof(struct msdos_boot_sector),
               "backup boot sector");
    }
  }
  /* seek to start of FATS and write them all */
  seekto(reserved_sectors * sector_size, "first FAT");
  for (x = 1; x <= nr_fats; x++) {
    int y;
    int blank_fat_length = fat_length - alloced_fat_length;
    writebuf(fat, alloced_fat_length * sector_size, "FAT");
    for (y = 0; y < blank_fat_length; y++)
      writebuf(blank_sector, sector_size, "FAT");
  }
  /* Write the root directory directly after the last FAT. This is the root
   * dir area on FAT12/16, and the first cluster on FAT32. */
  writebuf((char *)root_dir, size_root_dir, "root directory");

  if (blank_sector) free(blank_sector);
  if (info_sector) free(info_sector);
  free(root_dir); /* Free up the root directory space from setup_tables */
  free(fat);      /* Free up the fat table space reserved during setup_tables */
}

/* The "main" entry point into the utility - we pick up the options and attempt
   to process them in some sort of sensible
   way.  In the event that some/all of the options are invalid we need to tell
   the user so that something can be done! */

int format_device_fat(char *device, int fatsize, int sectorsize, char *label) {

  r_printf("mkfs.fat: Formatting %s, FAT%d, label: %s\n",
        device, fatsize, label);

  strcpy(volume_name, label);

  struct device_info devinfo;
  uint64_t cblocks = 0;
  struct timeval create_timeval;

  enum {
    OPT_HELP = 1000,
    OPT_INVARIANT,
  };

  device_name = device;
  program_name = "mkfs.fat";
  verbose = 1;

  gettimeofday(&create_timeval, NULL);
  create_time = create_timeval.tv_sec;
  volume_id = (uint32_t)((create_timeval.tv_sec << 20) |
                         create_timeval.tv_usec); /* Default volume ID =
                                                     creation time, fudged for
                                                     more uniqueness */
  size_fat = fatsize;

  if (sectorsize != 512 && sectorsize != 1024 && sectorsize != 2048 &&
      sectorsize != 4096 && sectorsize != 8192 && sectorsize != 16384 &&
      sectorsize != 32768) {
    sector_size = devinfo.sector_size;
    r_printf("Bad sector size.");
    return -1;
  } else {
    sector_size = sectorsize;
  }

  if (sector_size > 4096)
      r_printf("Warning: Sector sizes largen than 4096 bytes are non-standard and may fail.");

  if ((dev = open(device, O_EXCL | O_RDWR)) < 0) {
    r_printf("%s: unable to open %s: %s\n", program_name, device,
            strerror(errno));
    return -1;
  }

  if (get_device_info(dev, &devinfo) < 0)
    die("error collecting information about %s");

  if (devinfo.size <= 0) die("unable to discover size of %s");

  if (devinfo.sector_size > 0) sector_size = devinfo.sector_size;

  cblocks = devinfo.size / BLOCK_SIZE;
  orphaned_sectors = (devinfo.size % BLOCK_SIZE) / sector_size;
  blocks = cblocks;

  if (devinfo.type == TYPE_FIXED && devinfo.partition == 0)
    die("Device partition expected, not making filesystem on entire device "
        "'%s' (use -I to override)");

  if (devinfo.has_children > 0)
    die("Partitions or virtual mappings on device '%s', not making filesystem "
        "(use -I to override)");

  if (sector_size > 4096) r_printf("sector size too small");

  establish_params(&devinfo);
  setup_tables(); /* Establish the filesystem tables */

  if (check) check_blocks();

  write_tables(); /* Write the filesystem tables away! */

  if (close(dev) < 0) {
      return -1;
  }

  return 0; /* Terminate with no errors! */
}
