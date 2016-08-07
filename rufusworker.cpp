#include "rufusworker.h"
#include "log.h"

extern "C" {
    #include "linux_devices/partition.h"
}


RufusWorker::RufusWorker() : QThread()
{
}

void RufusWorker::run()
{
    r_printf((char*)"*** rufusl: Nuking and partitioning drive... \n");
    if (nuke_and_partition("/dev/sdb", TABLE_MBR, FS_NTFS) < 0) {
        r_printf((char*)"*** rufusl: Partitioning failed!\n");
    } else {
        r_printf((char*)"*** rufusl: OK");
    }
}
