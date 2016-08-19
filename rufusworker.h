#ifndef RUFUSWORKER_H
#define RUFUSWORKER_H

#include "QThread"
#include "log.h"
#include "linux/devices.h"

class RufusWorker : public QThread
{

    Q_OBJECT

private:
    int cluster_size;
    int file_system;
    int partition_scheme;
    int full_format;
    const QString *isopath;

public:

    RufusWorker(Device *chosen,
                int partition_scheme,
                int file_system,
                int cluster_size,
                int full_format,
                const QString *isopath);

    Device *theOne;
    void run();
};

#endif // RUFUSWORKER_H
