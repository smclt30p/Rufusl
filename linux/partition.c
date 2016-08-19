#include <errno.h>
#include <parted/parted.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "log.h"
#include "partition.h"
#include "definitions.h"

#define ASSERT(x, y)  \
  if (x == NULL) {    \
    r_printf(y);      \
    return -1;        \
  }
#define ASSERT_(x, y) \
  if (x == 0) {       \
    r_printf(y);      \
    return -1;        \
  }

/* WARNING: GNU chose the integer 0 to indicate an error, the code
   below is OK! Why they did that is beyond me. */

int nuke_and_partition(const char *path_dev, const int table, const int fs) {
  r_printf("Using device: %s\n", path_dev);

  PedDevice *device;
  PedDisk *disk;
  PedPartition *part;
  PedConstraint *constr;

  PedFileSystemType *fstype;
  PedDiskType *type;

  set_progress_bar(10);

  switch (fs) {
    case FS_NTFS:
      fstype = ped_file_system_type_get(NTFS);
      r_printf("* Marking partition type as NTFS\n");
      break;
    case FS_FAT32:
      fstype = ped_file_system_type_get(FAT32);
      r_printf("* Marking partition type as FAT32\n");
      break;
    default:
      r_printf("Internal error: unknown fs type.\n");
      return -1;
  }

  ASSERT(fstype,
         "Your system does not appear to be supporting FAT32/NTFS. Please "
         "install ntfs-3g, mkfs.fat or related.\n");
  set_progress_bar(20);
  device = ped_device_get(path_dev);
  ASSERT(device, "Opening device failed.\n");
  set_progress_bar(30);
  constr = ped_device_get_constraint(device);
  ASSERT(constr,
         "Failed to get device info. This *could* happen if your USB disk is a "
         "4K sector model. Try a different flash drive.\n");

  set_progress_bar(40);
  switch (table) {
    case TB_GPT:
      type = ped_disk_type_get(GPT);
      r_printf("* Marking partition table GUID/GPT\n");
      break;
    case TB_MBR:
      type = ped_disk_type_get(MBR);
      r_printf("* Marking partition table BIOS/MBR\n");
      break;
    default:
      r_printf("Internal error: unknown partition table.\n");
      return -1;
  }

  ASSERT(type,
         "libparted internal error: MBR/GPT Partition table info not found.\n");
  disk = ped_disk_new_fresh(device, type);
  set_progress_bar(50);
  ASSERT(disk, "Failed to nuke the USB. Full RAM?\n");
  part =
      ped_partition_new(disk, PED_PARTITION_NORMAL, fstype, 1, device->length);
  ASSERT(part, "Failed to construct partition Full RAM?\n");

  set_progress_bar(60);

  if (ped_partition_is_flag_available(part, PED_PARTITION_BOOT)) {
    r_printf("* Marking partition bootable\n");
    ped_partition_set_flag(part, PED_PARTITION_BOOT, 1);
  } else {
    r_printf("WARNING: Could not set bootable flag on partition!\n");
  }

  set_progress_bar(70);

  ASSERT_(ped_disk_add_partition(disk, part, constr),
          "Failed to add partition to MBR\n");
  r_printf("Writing partition table to MBR on %s\n", path_dev);
  set_progress_bar(80);

  ASSERT_(ped_disk_commit_to_dev(disk),
          "FATAL ERROR: FAILED TO WRITE MBR TO DEVICE!\n");
  r_printf("Refreshing kernel device partition info\n");
  set_progress_bar(90);
  ASSERT_(ped_disk_commit_to_os(disk),
          "Error informing kernel of new partition!\n");
  set_progress_bar(100);
  ped_device_free_all();

  sync();

  return 0;
}

int full_wipe(const uint32_t *device_fd) {

  set_progress_bar(0);

  size_t file_size;

  if (ioctl(*device_fd, BLKGETSIZE, &file_size) < 0) {
    perror("ioctl");
    return -1;
  }

  file_size += file_size * 512;
  r_printf("Fully wiping %ld bytes on fd %d\n", file_size, *device_fd);

  char buffer[4096];

  for (uint64_t i = 0; i < sizeof(buffer); i++) {
    buffer[i] = 0x00;
  }

  ssize_t temp;
  size_t temp2 = 0;
  double copied = 0;

  while ((temp = write(*device_fd, buffer, 4096)) > 0) {

    copied += temp;
    temp2 += temp;

    if (temp2 > 50000000) {
      set_progress_bar((int)(((file_size - (file_size - copied)) / file_size) * 100.0f));
      temp2 = 0;
    }

  }

  sync();

  return 0;
}
