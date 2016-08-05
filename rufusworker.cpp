#include "rufusworker.h"
#include "log.h"

extern "C" {
    #include "dosfstools/mkfs.fat.h"
}


RufusWorker::RufusWorker() : QThread()
{
}

void RufusWorker::run()
{
    r_printf((char*)"rufusl: Formatting device FAT32 ... \n");
    if (format_device_fat((char*)"/dev/sdb1", 32, 4096,(char*) "GALA") < 0) {
        r_printf((char*)"rufusl: Formatting failed!\n");
    } else {
        r_printf((char*)"rufusl: OK");
    }
}
