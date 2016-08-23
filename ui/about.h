#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>

namespace Ui {
class About;
}

class About : public QDialog
{
    Q_OBJECT

private:
    void reject();

public:
    explicit About(QWidget *parent = 0);
    static bool isOpen;
    ~About();

private slots:
    void on_buttonClose_clicked();

private:
    Ui::About *ui;
};

#endif // ABOUT_H
