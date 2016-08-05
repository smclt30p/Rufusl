#ifndef RUFUSWORKER_H
#define RUFUSWORKER_H

#include "QThread"
#include "log.h"


class RufusWorker : public QThread
{

    Q_OBJECT

public:
    RufusWorker();
    void run();
};

#endif // RUFUSWORKER_H
