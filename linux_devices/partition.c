#include "partition.h"
#include <errno.h>
#include <parted/parted.h>
#include "log.h"

#define ASSERT(x, y) if (x == NULL) { r_printf(y); return -1; }
#define ASSERT_(x, y) if (x == 0) { r_printf(y); return -1; }

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

  ASSERT(fstype, "Your system does not appear to be supporting FAT32/NTFS. Please install ntfs-3g, mkfs.fat or related.\n");
  device = ped_device_get(path_dev);
  ASSERT(device, "Opening device failed.\n");
  constr = ped_device_get_constraint(device);
  ASSERT(constr, "Failed to get device info. This *could* happen if your USB disk is a 4K sector model. Try a different flash drive.\n");

  switch (table) {
    case TABLE_GPT:
      type = ped_disk_type_get(GPT);
      r_printf("* Marking partition table GUID/GPT\n");
      break;
    case TABLE_MBR:
      type = ped_disk_type_get(MBR);
      r_printf("* Marking partition table BIOS/MBR\n");
      break;
    default:
      r_printf("Internal error: unknown partition table.\n");
      return -1;
  }

  ASSERT(type, "libparted internal error: MBR/GPT Partition table info not found.\n");
  disk = ped_disk_new_fresh(device, type);
  ASSERT(disk,"Failed to nuke the USB. Full RAM?\n");
  part = ped_partition_new(disk, PED_PARTITION_NORMAL, fstype, 1, device->length);
  ASSERT(part, "Failed to construct partition Full RAM?\n");

  if (ped_partition_is_flag_available(part, PED_PARTITION_BOOT)) {
    r_printf("* Marking partition bootable\n");
    ped_partition_set_flag(part, PED_PARTITION_BOOT, 1);
  } else {
    r_printf("WARNING: Could not set bootable flag on partition!\n");
  }

  ASSERT_(ped_disk_add_partition(disk, part, constr), "Failed to add partition to MBR\n");

  r_printf("Writing partition table to MBR on %s\n", path_dev);

  ASSERT_(ped_disk_commit_to_dev(disk) ,"FATAL ERROR: FAILED TO WRITE MBR TO DEVICE!\n");

  ped_device_free_all();

  return 0;
}
