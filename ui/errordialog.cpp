#include "errordialog.h"
#include "ui_errordialog.h"

ErrorDialog::ErrorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ErrorDialog)
{
    ui->setupUi(this);
}

ErrorDialog::~ErrorDialog()
{
    delete ui;
}

void ErrorDialog::warning(const char *msg) {

    this->ui->label->setText(msg);
    this->ui->cancel->setVisible(false);
    this->ui->retry->setVisible(false);
    this->show();

}

void ErrorDialog::on_ok_clicked()
{
    this->hide();
}
