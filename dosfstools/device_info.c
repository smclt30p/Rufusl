/* device_info.c - Collect device information for mkfs.fat

   Copyright (C) 2015 Andreas Bombe <aeb@debian.org>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "blkdev/blkdev.h"
#include "device_info.h"


static const struct device_info device_info_clueless = {
    .type         = TYPE_UNKNOWN,
    .partition    = -1,
    .has_children = -1,
    .geom_heads   = -1,
    .geom_sectors = -1,
    .geom_start   = -1,
    .sector_size  = -1,
    .size         = -1,
};


int device_info_verbose;


static void get_block_device_size(struct device_info *info, int fd)
{
    unsigned long long bytes;

    if (!blkdev_get_size(fd, &bytes) && bytes != 0)
	info->size = bytes;
}


static void get_block_geometry(struct device_info *info, int fd)
{
    unsigned int heads, sectors, start;

    if (!blkdev_get_geometry(fd, &heads, &sectors)
	    && heads && sectors) {
	info->geom_heads = heads;
	info->geom_sectors = sectors;
    }

    if (!blkdev_get_start(fd, &start))
	info->geom_start = start;
}


static void get_sector_size(struct device_info *info, int fd)
{
    int size;

    if (!blkdev_get_sector_size(fd, &size))
	info->sector_size = size;
}

int get_device_info(int fd, struct device_info *info)
{
    struct stat stat;
    int ret;

    *info = device_info_clueless;

    ret = fstat(fd, &stat);
    if (ret < 0) {
	perror("fstat on target failed");
	return -1;
    }

    if (S_ISREG(stat.st_mode)) {
	/* there is nothing more to discover for an image file */
	info->type = TYPE_FILE;
	info->partition = 0;
	info->size = stat.st_size;
	return 0;
    }

    if (!S_ISBLK(stat.st_mode)) {
	/* neither regular file nor block device? not usable */
	info->type = TYPE_BAD;
	return 0;
    }

    get_block_device_size(info, fd);
    get_block_geometry(info, fd);
    get_sector_size(info, fd);

    return 0;
}


int is_device_mounted(const char *path)
{

    return 0;

    /* This is handled by Rufusl */

    (void) path; /* prevent unused parameter warning */
    return 0;
}
