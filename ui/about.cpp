#include "about.h"
#include "ui_about.h"

#include <QDebug>

bool About::isOpen;

void About::reject()
{
    About::isOpen = false;
    QDialog::reject();
}

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);
}

About::~About()
{
    delete ui;
}

void About::on_buttonClose_clicked()
{
    About::isOpen = false;
    this->close();
    delete ui;
}
