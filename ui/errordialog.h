#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#include <QDialog>

namespace Ui {
class ErrorDialog;
}

class ErrorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ErrorDialog(QWidget *parent = 0);
    ~ErrorDialog();
    void warning(const char *msg);

private slots:
    void on_ok_clicked();

private:
    Ui::ErrorDialog *ui;
};

#endif // ERRORDIALOG_H
