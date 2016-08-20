#include <cstdio>

#include <QDebug>

#include "rufuswindow.h"
#include "ui_rufuswindow.h"
#include "log.h"
#include "definitions.h"

extern "C" {
    #include "linux/user.h"
}

#define SKIP_ROOT_CHECK

#define RUFUSL_VERSION "0.1.20 Alpha"
#define MKFS_VERSION "4.0"

Log *logptr; /* Forward declaration */

RufusWindow::RufusWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::RufusWindow) {

  ui->setupUi(this);

  this->setupUi();
  this->scan();
  this->show();

#ifndef SKIP_ROOT_CHECK
  if (!check_root()) {
      this->dialog->warning("WARNING: Rufusl has been run as an unprivileged user. Please re-run as root.\n");
      this->ui->container->setEnabled(false);
    r_printf("*** WARNING: Rufusl has been run as an unprivileged user.\n");
  } else {
      r_printf("*** Superuser: OK.\n");
  }
#endif

}

void RufusWindow::on_buttonClose_clicked() {
  if (Log::logOpen) {
    this->log->close();
  }
  if (About::isOpen) {
    this->about->close();
  }
  close();
}

void RufusWindow::on_buttonLog_clicked() {

  if (Log::logOpen) return;
  Log::logOpen = true;

  int resultsY = this->y();
  int resultsX = this->x() + this->log->frameGeometry().width() - 50;

  this->log->move(resultsX, resultsY);
  this->log->show();

}

void RufusWindow::on_buttonAbout_clicked() {

  if (About::isOpen) return;

  About::isOpen = true;

  this->about = new About();
  this->about->show();

}

void RufusWindow::on_buttonStart_clicked() {

    if (this->iso_path == NULL) {
        this->dialog->warning("No image selected.");
        return;
    }

    if (this->discovered == 0) {
        this->dialog->warning("No removable SCSI devices found.");
        return;
    }

    int index = this->box->currentIndex();
    this->worker = new RufusWorker(&devices[index],
                                   ui->partitionCombo->currentIndex(),
                                   ui->fsCombo->currentIndex(),
                                   ui->clusterCombo->currentIndex(),
                                   ui->formatCheck->isChecked(),
                                   this->iso_path,
                                   JOB_COPY);
    this->worker->start();

}

void RufusWindow::setProgress(int perc) {
    this->ui->progressBar->setValue(perc);
}

void RufusWindow::setupUi()
{
    /* Set up logger */

    this->log = new Log();
    this->log->set_up(this->ui->progressBar, this->ui->statusEdit);
    this->dialog = new ErrorDialog();

    /* Add DeviceComboBox to UI */

    this->box = new DeviceComboBox(this->ui->layoutWidget, this);
    this->box->setObjectName(QStringLiteral("deviceCombo"));
    this->ui->horizontalLayout->addWidget(box);

    /* Set up logptr in log.h so that
     * we can access the logger from C
     * source files */

    logptr = this->log;

    r_printf("*** Rufus version %s\n*** mkfs.fat version %s\n", RUFUSL_VERSION, MKFS_VERSION);

    /* Add items to 'Cluster size' field */

    this->ui->clusterCombo->addItem(BS_512B_LABEL);
    this->ui->clusterCombo->addItem(BS_1024B_LABEL);
    this->ui->clusterCombo->addItem(BS_2048B_LABEL);
    this->ui->clusterCombo->addItem(BS_4096B_LABEL);
    this->ui->clusterCombo->addItem(BS_8192B_LABEL);
    this->ui->clusterCombo->addItem(BS_16384B_LABEL);
    this->ui->clusterCombo->addItem(BS_32768B_LABEL);

    this->ui->clusterCombo->setCurrentIndex(BS_4096B);

    /* Add items to 'File system' */

    this->ui->fsCombo->addItem(FS_FAT32_LABEL);
    this->ui->fsCombo->addItem(FS_NTFS_LABEL);
    this->ui->fsCombo->setEnabled(false);

    /* Add items to partition table items */

    this->ui->partitionCombo->addItem(TB_MBR_LABEL);
    this->ui->partitionCombo->addItem(TB_GPT_LABEL);

    /* Add items to source combo */

    this->ui->sourceCombo->addItem(SRC_ISO_LABEL);
    this->ui->sourceCombo->addItem(SRC_DD_LABEL);

}


/* Scan SCSI Devices and update UI */

void RufusWindow::scan() {

  /* Clear combo box and reset number of discovered devices */

  this->box->clear();
  this->discovered = 0;

  /* Clear array */

  for (int i = 0; i < MAX_DEVICES; i++) {
    this->devices[i].capacity = 0;
    if (memset(this->devices[i].device, 0, sizeof(this->devices[i].device)) ==
        NULL)
      return;
    if (memset(this->devices[i].model, 0, sizeof(this->devices[i].model)) ==
        NULL)
      return;
    if (memset(this->devices[i].vendor, 0, sizeof(this->devices[i].vendor)) ==
        NULL)
      return;
  }

  /* Scan for devices */

  scan_devices(this->devices, MAX_DEVICES, &this->discovered);

  /* Format device names and add to combo box */

  char buf[255];

  for (int i = 0; i < this->discovered; i++) {

    snprintf(buf, sizeof(buf), "%s %s (%s) [%.1lf GB]", devices[i].vendor,
             devices[i].model, devices[i].device, devices[i].capacity * 512 / 1000000000.0f);

    r_printf("Found %s, major: %d, minor: %d\n", buf, devices[i].major, devices[i].minor);

    this->box->addItem(buf);

    if (memset(buf, 0, sizeof(buf)) == NULL) return;

  }
}

RufusWindow::~RufusWindow() {
  delete box;
  delete log;
  delete ui;
}

void RufusWindow::on_usingSearch_clicked()
{
    file_dialog = new QFileDialog;
    this->iso_path = new QString(file_dialog->getOpenFileName());
    file_dialog->close();
    if (this->iso_path->size() == 0) return;
    this->worker = new RufusWorker(NULL, 0xFF, 0xFF, 0xFF, 0xFF, this->iso_path, JOB_SCAN);
    this->worker->start();

    // RufusWorker scan_iso() ...
}
