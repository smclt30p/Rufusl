#ifndef DEVICECOMBOBOX_H
#define DEVICECOMBOBOX_H

#include <QComboBox>
#include "rufuswindow.h"

class RufusWindow;

namespace Ui {
class DeviceComboBox;
}

class DeviceComboBox : public QComboBox {
public:
    DeviceComboBox(QWidget*&, RufusWindow*);
    RufusWindow *main;
    void showPopup();
};

#endif // DEVICECOMBOBOX_H
