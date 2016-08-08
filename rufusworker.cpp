#include "rufusworker.h"
#include "log.h"

extern "C" {
#include "linux_devices/mounting.h"
#include "linux_devices/partition.h"
#include "dosfstools/mkfs.fat.h"
}

#define ASSERT(x, y)\
    r_printf(y); \
    set_ticker(y); \
    if (x < 0) { \
        r_printf("*** Failed.\n"); \
        set_ticker("FAILED"); \
        return; \
    } else { \
        r_printf("*** OK!\n"); \
        add_progress_bar(1); \
        set_ticker("OK"); \
    } \

RufusWorker::RufusWorker(Device *chosen) : QThread() {
    this->theOne = chosen;
    r_printf("Using %s\n major: %d\n minor: %d\n", theOne->device, theOne->major, theOne->minor);
}

void RufusWorker::run() {

  ASSERT(make_temp_dir(),                                               "STEP(1/5) - Creating temporary directory\n");
  ASSERT(make_temp_device(theOne->major, theOne->minor),                "STEP(2/5) - Creating temporary device node\n");
  ASSERT(nuke_and_partition(TEMP_DEVICE, TABLE_MBR, FS_NTFS),           "STEP(3/5) - Nuking and partitioning drive\n");
  ASSERT(make_temp_partition(theOne->major, theOne->minor),             "STEP(4/5) - Creating temporary partition node\n");
  ASSERT(format_device_fat((char*)TEMP_PART, 32, 4096, (char*) "GALA"), "STEP(5/5) - Formatting\n");

}


