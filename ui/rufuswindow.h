#ifndef RUFUSWINDOW_H
#define RUFUSWINDOW_H

#include <QMainWindow>
#include <QFileDialog>

extern "C" {

#include <stdint.h>
#include "linux/devices.h"

}

#include "log.h"
#include "about.h"
#include "devicecombobox.h"
#include "rufusworker.h"
#include "errordialog.h"

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
    QString *iso_path;
    void scan();
    ~RufusWindow();


private slots:

    void on_buttonClose_clicked();
    void on_buttonLog_clicked();
    void on_buttonAbout_clicked();
    void on_buttonStart_clicked();
    void setProgress(int);

    void on_usingSearch_clicked();

private:

    Ui::RufusWindow *ui;
    Log *log;
    About *about;
    Device devices[MAX_DEVICES];
    uint8_t discovered = 0;
    DeviceComboBox *box;
    RufusWorker *worker;
    ErrorDialog *dialog;
    QFileDialog *file_dialog;

    void setupUi();

    signals:
        void log_write(char *);

};

#endif // RUFUSWINDOW_H
