#ifndef RUFUSWINDOW_H
#define RUFUSWINDOW_H

#include <QMainWindow>

extern "C" {

#include <stdint.h>
#include "linux_devices/devices.h"

}

#include "log.h"
#include "about.h"
#include "devicecombobox.h"
#include "rufusworker.h"

#define MAX_DEVICES 32

namespace Ui {
class RufusWindow;
}

class DeviceComboBox; /* Forward declaration to solve compiler confusion */

class RufusWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit RufusWindow(QWidget *parent = 0);
    void scan();
    ~RufusWindow();


private slots:

    void on_buttonClose_clicked();
    void on_buttonLog_clicked();
    void on_buttonAbout_clicked();
    void on_buttonStart_clicked();
    void setProgress(int);

private:

    Ui::RufusWindow *ui;
    Log *log;
    About *about;
    Device devices[MAX_DEVICES];
    uint8_t discovered = 0;
    DeviceComboBox *box;
    RufusWorker *worker;

    void setupUi();

    signals:
        void log_write(char *);

};

#endif // RUFUSWINDOW_H
