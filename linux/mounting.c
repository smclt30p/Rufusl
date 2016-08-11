#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../log.h"
#include "mounting.h"

int make_temp_device(uint8_t major, uint8_t minor) {
  remove(TEMP_DEVICE);
  remove(TEMP_PART);

  r_printf("Creating temporary node for Rufusl: major: %d minor %d\n", major,
           minor);

  dev_t temp_dev = makedev(major, minor);

  if (temp_dev < 0) return -1;

  if (mknod(TEMP_DEVICE, S_IFBLK, temp_dev) < 0) {
    r_printf("Creating temporaray device node failed: %s\n", strerror(errno));
    return -1;
  }
}

int make_temp_partition(uint8_t major, uint8_t minor) {
  minor++;

  r_printf("Creating temporary partition node for Rufusl: major: %d minor %d\n",
           major, minor);

  dev_t temp_dev = makedev(major, minor);

  if (temp_dev < 0) return -1;

  if (mknod(TEMP_PART, S_IFBLK, temp_dev) < 0) {
    r_printf("Creating temporaray device node failed: %s\n", strerror(errno));
    return -1;
  }
}

int make_temp_dir() {

  if (umount(TEMP_DIR) < 0) {
    r_printf("Error: %s\n", strerror(errno));
  }

  struct stat st = {0};
  int i = 0;

  if (stat(TEMP_DIR, &st) == -1) {
    r_printf("Creating temporary directory...\n");
    mkdir(TEMP_DIR, 0700);
  } else {

    DIR *dir;
    struct dirent *ent;

    dir = opendir(TEMP_DIR);

    if (dir == NULL) {
        r_printf("Failed to load tmpfs.\n");
        return -1;
    }

    while (ent = readdir(dir)) {
      if (ent->d_name[0] == '.') continue;
      i++;
    }

    if (i == 0) {
      remove(TEMP_DIR);
      if (stat(TEMP_DIR, &st) == -1) {
        r_printf("Creating temporary directory...\n");
        mkdir(TEMP_DIR, 0700);
      }

    } else {
      r_printf(
          "*** WARNING: Temporary directory is not mounted, and is not "
          "empty!\n");
    }

    closedir(dir);

    return 0;
  }
}
