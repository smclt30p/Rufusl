#include "devicecombobox.h"

DeviceComboBox::DeviceComboBox(QWidget *& w, RufusWindow *window) : QComboBox(w)
{
    this->main = window;
}

void DeviceComboBox::showPopup() {
    this->main->scan();
    QComboBox::showPopup();
}
