#include <stdint.h>

#include "rufusworker.h"
#include "log.h"
#include "definitions.h"

extern "C" {
#include "linux/mounting.h"
#include "linux/partition.h"
#include "linux/fat32.h"
#include "iso.h"
}

#define ASSERT(x)\
    set_progress_bar(0); \
    if (x < 0) { \
        set_ticker("FAILED"); \
        set_progress_bar(0); \
        clean_up(&device_fd, &part_fd, &loop_fd, &iso_fd); \
        return; \
    }

RufusWorker::RufusWorker(Device *chosen,
                         int partition_scheme,
                         int file_system,
                         int cluster_size,
                         int full_format,
                         const QString *isopath_,
                         uint8_t job_type_) : QThread() {

    this->theOne = chosen;
    this->cluster_size = cluster_size;
    this->partition_scheme = partition_scheme;
    this->file_system = file_system;
    this->full_format = full_format;
    this->isopath = isopath_;
    this->job_type = job_type_;

}

void RufusWorker::run() {


    uint32_t device_fd;
    uint32_t part_fd;
    uint32_t loop_fd;
    uint32_t iso_fd;

 switch(job_type) {
 case JOB_COPY:

     r_printf("Using %s\n major: %d\n minor: %d\n", theOne->device, theOne->major, theOne->minor);

     set_ticker("Warming up...");

     ASSERT(make_temp_dir(TEMP_DIR));
     ASSERT(make_temp_dir(TEMP_DIR_ISO));
     ASSERT(make_loop_device(&loop_fd));
     ASSERT(make_temp_device(theOne->major, theOne->minor, &device_fd));

     if (!full_format) {
        set_ticker("Running full format...");
        ASSERT(full_wipe(&device_fd));
     }

     set_ticker("Partitioning drive...");

     ASSERT(nuke_and_partition(TEMP_DEVICE, this->partition_scheme, this->file_system));
     ASSERT(make_temp_partition(theOne->major, theOne->minor, &part_fd));
     ASSERT(format_fat32(&part_fd, this->cluster_size, (char*) "GALA"));
     ASSERT(mount_device_to_temp(&file_system));
     ASSERT(mount_iso_to_loop(this->isopath->toStdString().c_str(), this->isopath->size(), &loop_fd, &iso_fd)); /* QString is garbage. */

     set_ticker("Copying data to USB...");

     ASSERT(recursive_copy( (char*) TEMP_DIR_ISO, (char*) TEMP_DIR));

     set_ticker("Cleaning up...");

     clean_up(&device_fd, &part_fd, &loop_fd, &iso_fd);

     set_ticker("DONE");

     this->theOne = NULL;
     this->cluster_size  = 255;
     this->partition_scheme  = 255;
     this->file_system  = 255;
     this->full_format  = 255;
     this->isopath  = NULL;

     break;

 case JOB_SCAN:

     set_ticker("Analyzing ISO Image...");
     r_printf("Analyzing ISO Image");

     ASSERT(make_temp_dir(TEMP_DIR_ISO));
     ASSERT(make_loop_device(&loop_fd));
     ASSERT(mount_iso_to_loop(this->isopath->toStdString().c_str(), this->isopath->size(), &loop_fd, &iso_fd)); /* QString is garbage. */
     ASSERT(recursive_iso_scan(&loop_fd));

     clean_up(&device_fd, &part_fd, &loop_fd, &iso_fd);

     break;
 default:
     r_printf("Invalid job type!");
 }

}


