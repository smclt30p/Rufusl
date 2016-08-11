#ifndef RUFUSWORKER_H
#define RUFUSWORKER_H

#include "QThread"
#include "log.h"
#include "linux/devices.h"

class RufusWorker : public QThread
{

    Q_OBJECT

public:
    RufusWorker(Device *chosen);
    Device *theOne;
    void run();
};

#endif // RUFUSWORKER_H
