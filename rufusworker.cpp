#include <stdint.h>

#include "rufusworker.h"
#include "log.h"
#include "definitions.h"

extern "C" {
#include "linux/mounting.h"
#include "linux/partition.h"
#include "linux/fat32.h"
}

#define ASSERT(x, y)\
    set_progress_bar(0); \
    r_printf(y); \
    set_ticker(y); \
    if (x < 0) { \
        r_printf("*** Failed.\n"); \
        set_ticker("FAILED"); \
        set_progress_bar(0); \
        clean_up(&device_fd, &part_fd, &loop_fd); \
        return; \
    } else { \
        r_printf("*** OK!\n"); \
        set_progress_bar(100);\
        set_ticker("OK"); \
    } \

RufusWorker::RufusWorker(Device *chosen,
                         int partition_scheme,
                         int file_system,
                         int cluster_size,
                         int full_format,
                         const QString *isopath_) : QThread() {

    this->theOne = chosen;
    this->cluster_size = cluster_size;
    this->partition_scheme = partition_scheme;
    this->file_system = file_system;
    this->full_format = full_format;
    this->isopath = isopath_;

    r_printf("Using %s\n major: %d\n minor: %d\n", theOne->device, theOne->major, theOne->minor);

}

void RufusWorker::run() {

    uint32_t device_fd;
    uint32_t part_fd;
    uint32_t loop_fd;

  ASSERT(make_temp_dir(TEMP_DIR),                                                                  "STEP(1/11) - Creating temporary directory\n");
  ASSERT(make_temp_dir(TEMP_DIR_ISO),                                                              "STEP(2/11) - Creating temporary directory\n");
  ASSERT(make_loop_device(&loop_fd),                                                               "STEP(3/11) - Creating loop node\n");
  ASSERT(make_temp_device(theOne->major, theOne->minor, &device_fd),                               "STEP(4/11) - Creating temporary device node\n");

  if (!full_format) {
     ASSERT(full_wipe(&device_fd),                                                                 "STEP(5/11) - Full format\n");
  }

  ASSERT(nuke_and_partition(TEMP_DEVICE, this->partition_scheme, this->file_system),               "STEP(6/11) - Partitioning drive\n");
  ASSERT(make_temp_partition(theOne->major, theOne->minor, &part_fd),                              "STEP(7/11) - Creating temporary partition node\n");
  ASSERT(format_fat32(&part_fd, this->cluster_size, (char*) "GALA"),                               "STEP(8/11) - Formatting\n");
  ASSERT(mount_device_to_temp(&file_system),                                                       "STEP(9/11) - Mounting device to temp\n");
  ASSERT(mount_iso_to_loop(this->isopath->toStdString().c_str(), this->isopath->size(), &loop_fd), "STEP(10/11) - Mounting ISO\n"); /* QString is garbage. */
  ASSERT(recursive_copy( (char*) TEMP_DIR_ISO, (char*) TEMP_DIR),                                  "STEP(11/11) - Copying ISO to USB\n");

  clean_up(&device_fd, &part_fd, &loop_fd);

}


