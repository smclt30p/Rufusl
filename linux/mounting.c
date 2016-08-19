#define _XOPEN_SOURCE 500
#define _BSD_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <linux/loop.h>
#include <sys/types.h>
#include <unistd.h>
#include <ftw.h>

#include "../log.h"
#include "mounting.h"
#include "definitions.h"

#define BUF_SIZE 4096

static char *source;
static char *dest;
static long file_count = 0;
static long files_copied = 0;

int make_temp_device(uint8_t major, uint8_t minor, uint32_t *device_fd) {

  remove(TEMP_DEVICE);

  r_printf("Creating temporary node for Rufusl: major: %d minor %d\n", major,
           minor);

  dev_t temp_dev = makedev(major, minor);

  if (temp_dev < 0) return -1;

  if (mknod(TEMP_DEVICE, S_IFBLK, temp_dev) < 0) {
    r_printf("Creating temporaray device node failed: %s\n", strerror(errno));
    return -1;
  }

  r_printf("Opening device for writing ... ");

  if ((*device_fd = open(TEMP_DEVICE, O_RDWR)) < 0) {
      r_printf("Error opening: %s\n", strerror(errno));
      return -1;
  }

  r_printf(" OK! fd: %d\n", *device_fd);
}

int make_temp_partition(uint8_t major, uint8_t minor, uint32_t *part_fd) {
  minor++;

  r_printf("Creating temporary partition node for Rufusl: major: %d minor %d\n",
           major, minor);

  dev_t temp_dev = makedev(major, minor);

  if (temp_dev < 0) return -1;

  if (mknod(TEMP_PART, S_IFBLK, temp_dev) < 0) {
    r_printf("Creating temporaray device node failed: %s\n", strerror(errno));
    return -1;
  }

  r_printf("Opening partition for writing ... ");

  if ((*part_fd = open(TEMP_PART, O_RDWR)) < 0) {
      r_printf("Error opening: %s\n", strerror(errno));
      return -1;
  }

  r_printf(" OK! fd: %d\n", *part_fd);

}

int make_temp_dir(const char *path) {

  umount(path);

  struct stat st = {0};
  int i = 0;

  if (stat(path, &st) == -1) {
    r_printf("Creating temporary directory in %s\n", path);
    mkdir(path, 0700);
  } else {
    DIR *dir;
    struct dirent *ent;

    dir = opendir(path);

    if (dir == NULL) {
      r_printf("Failed to load tmpfs: %s\n", strerror(errno));
      return -1;
    }

    while (ent = readdir(dir)) {
      if (ent->d_name[0] == '.') continue;
      i++;
    }

    if (i == 0) {
      remove(path);
      if (stat(path, &st) == -1) {
        r_printf("Creating temporary directory...\n");
        mkdir(path, 0700);
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

int callback(const char *fpath, const struct stat *sb, int typeflag,
             struct FTW *ftwbuf) {

  /* Allocate new array and copy fpath into it because
     nftw() has an assertion if fpath has been changed. */

  char buf[strlen(fpath) + 1];
  memcpy(buf, fpath, strlen(fpath) + 1);

  /* Get length of path where the loop device is mounted */

  uint32_t a_len = strlen(source);

  /* Trim soure path. Write each char from (buf + a_len + 1) to buf. */

  uint32_t i = a_len + 1;
  uint32_t j = 0;

  for (; j < sizeof(buf) - a_len; i++) {
    buf[j] = *(buf + i);
    j++;
  }

  buf[j] = 0x00; /* Null-terminate the string. Now we have a relative path */

  /* Allocate new buffer with size of destination path, and the relative path +
   * a null terminator */

  char dest_path[strlen(dest) + strlen(buf) + 1];

  /* Concate destination and relative path */

  snprintf(dest_path, strlen(dest) + strlen(buf) + 1, "%s%s", dest, buf);

  /* Now we have an absolute source and destination path. :) */

  files_copied++;

  if (typeflag == FTW_D) {
    mkdir(dest_path, 0700);
    return 0;
  }

  int inputFd, outputFd, openFlags;
  mode_t filePerms;
  ssize_t numRead;

  r_printf("Extracting: %s\n", dest_path);
  set_progress_bar((int) ((( (float) file_count - ( (float) file_count - (float) files_copied)) / (float) file_count) * 100.0f));

  // set_progress_bar(  (file_count - (file_count - files_copied) / file_count) * 100.0f  );

  char buf1[BUF_SIZE];

  inputFd = open(fpath, O_RDONLY);

  if (inputFd == -1) {
      r_printf("Error: %s\n", strerror(errno));
      return -1;
  }

  openFlags = O_CREAT | O_WRONLY;
  filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

  outputFd = open(dest_path, openFlags, filePerms);

  if (outputFd == -1) {
    r_printf("Error: %s\n", strerror(errno));
    return -1;
  }

  while ((numRead = read(inputFd, buf1, BUF_SIZE)) > 0) {
    if (write(outputFd, buf1, numRead) != numRead) {
      r_printf("Error: %s\n", strerror(errno));
      break;
    }
  }

  if (numRead == -1) {
      r_printf("Error: %s\n", strerror(errno));
      return -1;
  }
  if (close(outputFd) == -1) {
      r_printf("Error: %s\n", strerror(errno));
      return -1;
  }
  if (close(inputFd) == -1) {
      r_printf("Error: %s\n", strerror(errno));
      return -1;
  }

  return 0;
}

int count_files(const char *fpath, const struct stat *sb, int typeflag,
             struct FTW *ftwbuf) {
    file_count++;
    return 0;
}

int recursive_copy( char *src,  char *dest_) {

    source = src;
    dest = dest_;

    files_copied = 0;
    file_count = 0;

    r_printf("Counting files...\n");
    nftw(source, count_files, 4, 0);
    nftw(source, callback , 4, 0);

}

void clean_up(const uint32_t *dev_fd, const uint32_t *part_fd, const uint32_t *loop_fd) {

    r_printf("Cleaning up...\n");

    sync();

    umount(TEMP_DIR);
    umount(TEMP_DIR_ISO);
    remove(TEMP_DIR);
    remove(TEMP_DIR_ISO);
    remove(TEMP_DEVICE);
    remove(TEMP_PART);
    remove(TEMP_LOOP);
    close(*part_fd);
    close(*dev_fd);
    ioctl(*loop_fd, LOOP_CLR_FD);
    close(*loop_fd);

    r_printf("Rufus finnished all tasks.\n");

}

int make_loop_device(uint32_t *loop_fd) {

    if (access(TEMP_LOOP, R_OK) == 0) {
        int file_fd = open(TEMP_LOOP, O_RDONLY);
        if (file_fd < 0) {
            r_printf("Loop error: %s\n", strerror(errno));
            return -1;
        }

        if (ioctl(file_fd, LOOP_CLR_FD) < 0) {
            r_printf("Loop device sane, skipping creation\n");

            if ((*loop_fd = open(TEMP_LOOP, O_RDONLY)) < 0) {
                r_printf("Error opening loop: %s\n", strerror(errno));
                return -1;
            }

            r_printf("Loop fd: %d\n", *loop_fd);

            return 0;
        }
    }

    if (remove(TEMP_LOOP) < 0) {
        r_printf("Loop clear error: %s\n", strerror(errno));
    }

    dev_t temp_dev = makedev(7, 0);

    if (temp_dev < 0) return -1;

    if (mknod(TEMP_LOOP, S_IFBLK, temp_dev) < 0) {
      r_printf("Creating loop node failed: %s\n", strerror(errno));
      return -1;
    }

    if ((*loop_fd = open(TEMP_LOOP, O_RDONLY)) < 0) {
        r_printf("Error opening loop: %s\n", strerror(errno));
        return -1;
    }

    r_printf("Loop OK, fd: %d\n", *loop_fd);

    return 0;

}

int mount_device_to_temp(const int32_t *file_system) {

  switch (*file_system) {
    case FS_FAT32:
      if (mount(TEMP_PART, TEMP_DIR, MOUNT_FAT32, MS_MGC_VAL, NULL) < 0) {
        r_printf("Device mount error: %s\n", strerror(errno));
        return -1;
      }
      r_printf("Mount OK!\n");
      break;
    case FS_NTFS:
      if (mount(TEMP_PART, TEMP_DIR, MOUNT_NTFS, MS_MGC_VAL, NULL) < 0) {
        r_printf("Device mount error: %s\n", strerror(errno));
        return -1;
      }
      r_printf("Mount OK\n");
      break;
  }

}

int mount_iso_to_loop(const char *isopath, int isopath_len, const uint32_t *loop_fd) {

    /* We need the lenght of the ISO path because QString is the !~~ FuTuRe ~~! (tm) (c) */

    char c_path[isopath_len + 1];
    memcpy(c_path, isopath, (size_t) isopath_len);
    c_path[isopath_len + 1] = 0x00;

    r_printf("Using ISO: %s\n", c_path);

   int32_t iso_fd = open(c_path, O_RDWR);

   if (iso_fd < 0) { r_printf("Opening iso failed: %s\n", strerror(errno)); return -1; }

   ioctl(*loop_fd, LOOP_CLR_FD);

   if (ioctl(*loop_fd, LOOP_SET_FD, iso_fd) < 0 ) {
       r_printf("ioctl loop error 1: %s\n", strerror(errno));
       close(iso_fd);
       return -1;
   }

   if (close(iso_fd) < 0) {
       r_printf("close iso error: %s\n", strerror(errno));
       return -1;
   }

   struct loop_info64 info;

   info.lo_offset = 0;
   info.lo_sizelimit = 0;
   info.lo_encrypt_type = 0;

   if (ioctl(*loop_fd, LOOP_SET_STATUS64, &info) < 0) {
       r_printf("ioctl loop error 2: %s\n", strerror(errno));
       return -1;
   }

   r_printf("Mounting ISO on %s ... ", TEMP_DIR_ISO);

   if (mount(TEMP_LOOP, TEMP_DIR_ISO, MOUNT_LOOP, MS_MGC_VAL | MS_RDONLY, NULL) < 0) {
       r_printf("ISO Mount failed: %s\n", strerror(errno));
       return -1;
   }

   r_printf("OK!\n");

   return 0;

}

