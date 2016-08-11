#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#include "devices.h"

#define SYSFS_BLOCK_DEVFILE "/sys/block/%s/dev"
#define SYSFS_BLOCK_REMOVABLE "/sys/block/%s/removable"
#define SYSFS_BLOCK_MODEL "/sys/block/%s/device/model"
#define SYSFS_BLOCK_VENDOR "/sys/block/%s/device/vendor"
#define SYSFS_BLOCK_SIZE "/sys/block/%s/size"

#define REMOVABLE '1'
#define SCSI_DEVICE '8'
#define MAX_DEVICES 32

int scan_devices(Device *dev, int array_size, uint8_t *discovered) {

    DIR *dp;
    struct dirent *ep;
    char major[10];
    char minor[10];
    int i, j, len;
    char devfile[150];
    int index = 0;

    dp = opendir("/sys/block/");

  if (dp != NULL) {

    while ((ep = readdir(dp))) {

        if (ep->d_name[0] == '.') continue;

        /* Wipe the buffer becuase of the loop,
           construct the path to SYSFS_BLOCK_DEVFILE
           and read the SYSFS_BLOCK_DEVFILE
           into the buffer */

        memset(devfile, 0, sizeof(devfile));
        if ((snprintf(devfile, sizeof(devfile), SYSFS_BLOCK_DEVFILE, ep->d_name)) < 0) return -1;
        if (buffread(devfile, sizeof(devfile), devfile) < 0) return -1;

        /* Check if the SYSFS_BLOCK_DEVFILE's
           major in the buffer indicates that it is
           an SCSI (USB) device */

        if (*devfile == SCSI_DEVICE) {

            /* Wipe the buffer clean, format the
               path to SYSFS_BLOCK_REMOVABLE and
               read SYSFS_BLOCK_REMOVABLE into the
               buffer */

            memset(devfile, 0, sizeof(devfile));
            if ((snprintf(devfile, sizeof(devfile), SYSFS_BLOCK_REMOVABLE , ep->d_name)) < 0) return -1;
            if (buffread(devfile, sizeof(devfile), devfile) < 0) return -1;

            /* Check if SYSFS_BLOCK_REMOVABLE in
               the buffer indidcates that the SCSI
               device is removable */

            if (*devfile == REMOVABLE) {

                /* Copy the current device sysfs
                   name into the indexth
                   Device struct */

                if (strcpy((*(dev + index)).device, ep->d_name) == NULL) continue;

                /* Wipe the buffer clean, and format
                   the path to SYSFS_BLOCK_VENDOR, then
                   read from SYSFS_BLOCK_VENDOR and write
                   into indexth Device struct vendor name, then
                   trim all trailing and leading whitespaces
                   including the newline character */

                memset(devfile, 0, sizeof(devfile));
                if ((snprintf(devfile, sizeof(devfile), SYSFS_BLOCK_VENDOR , ep->d_name)) < 0) return -1;
                if (buffread((*(dev + index)).vendor, sizeof((*(dev + index)).vendor), devfile) < 0) return -1;
                trimwhitespace((*(dev + index)).vendor);

                /* Wipe the buffer clean, and format
                   the path to SYSFS_BLOCK_MODEL, then
                   read from SYSFS_BLOCK_MODEL and write
                   into indexth Device struct model name, then
                   trim all trailing and leading whitespaces
                   including the newline character */

                memset(devfile, 0, sizeof(devfile));
                if ((snprintf(devfile, sizeof(devfile), SYSFS_BLOCK_MODEL , ep->d_name)) < 0) return -1;
                if (buffread((*(dev + index)).model, sizeof((*(dev + index)).model), devfile) < 0) return -1;
                trimwhitespace((*(dev + index)).model);

                /* Wipe the buffer clean, and format
                   the path to SYSFS_BLOCK_SIZE, then
                   read from SYSFS_BLOCK_SIZE and write into
                   the buffer, then convert the buffer string
                   it to an unsigned 64-bit integer ignoring
                   all garbage that strtol may produce, and
                   assign it to the Device struct capacity
                   property */

                memset(devfile, 0, sizeof(devfile));
                if ((snprintf(devfile, sizeof(devfile), SYSFS_BLOCK_SIZE , ep->d_name)) < 0) return -1;
                if (buffread(devfile, sizeof(devfile), devfile) < 0) return -1;
                (*(dev + index)).capacity = strtol(devfile, NULL, 10);


                /* Read devfile again to populate struct */

                memset(devfile, 0, sizeof(devfile));
                if ((snprintf(devfile, sizeof(devfile), SYSFS_BLOCK_DEVFILE , ep->d_name)) < 0) return -1;
                if (buffread(devfile, sizeof(devfile), devfile) < 0) return -1;

                /* Wipe buffer */

                memset(minor, 0, sizeof(minor));
                memset(major, 0, sizeof(major));

                len = strlen(devfile);

                /* Write digits up to ":" to buffer */

                for (i = 0; i < len; i++) {
                    if (devfile[i] == ':') break;
                    major[i] = devfile[i];
                }

                /* Skip ":" char */

                i++;

                /* Read after ":" into buffer */

                for (j = 0;i < len; i++) {
                    if (devfile[i] == '\n') break;
                    minor[j] = devfile[i];
                    j++;
                }

                /* Cover buffer to long and cast long to short */

                (*(dev + index)).minor = (uint8_t) strtol(minor, NULL, 10);
                (*(dev + index)).major = (uint8_t) strtol(major, NULL, 10);

                /* Increment number of discovered
                   devices and the index of the current
                   Device struct in the array */

                (*discovered)++;
                index++;

                /* If there is more device in /sys/block than
                   there is Device struct members, break the loop
                   ignoring the other members */

                if (index >= array_size) break;

        }
      }
    }

    closedir(dp);

  } else {
    perror("Couldn't open the directory");
  }


  return (*discovered) == 0 ? -1 : 0;
}

void trimwhitespace(char *str) {

  char *end;
  while(isspace(*str)) str++;
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;
  *(end+1) = 0;

}

int buffread(char *buf, int sizeofbuf, char *path) {

    int file_fd;

    if ((file_fd = open(path, O_RDONLY)) < 0) {
        perror("buffread");
        return -1;
    }

    memset(buf, 0, sizeofbuf);

    if (read(file_fd, buf, sizeofbuf) < 0) {
        perror("buffread");
        return -1;
    }

    if (close(file_fd) < 0) {
        perror("buffread");
        return -1;
    }

    return 0;
}
