#include "rufusworker.h"
#include "log.h"

extern "C" {
#include "linux_devices/mounting.h"
#include "linux_devices/partition.h"
#include "dosfstools/mkfs.fat.h"
}

#define ASSERT(x, y) r_printf(y); if (x < 0) { r_printf("*** Failed.\n"); return; } else { r_printf("*** OK!\n"); }

RufusWorker::RufusWorker() : QThread() {}

void RufusWorker::run() {

  ASSERT(make_temp_dir(),                                        "*** STEP(1/5) - Creating temporary directory\n");
  ASSERT(make_temp_device(8, 16),                                "*** STEP(2/5) - Creating temporary device node\n");
  ASSERT(nuke_and_partition(TEMP_DEVICE, TABLE_MBR, FS_NTFS),    "*** STEP(3/5) - Nuking and partitioning drive\n");
  ASSERT(make_temp_partition(8, 16),                             "*** STEP(4/5) - Creating temporary partition node\n");
  ASSERT(format_device_fat(TEMP_PART, 32, 4096, (char*) "GALA"), "*** STEP(5/5) - Formatting\n");

}


