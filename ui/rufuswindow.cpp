#include <cstdio>

#include "rufuswindow.h"
#include "ui_rufuswindow.h"
#include "log.h"

extern "C" {
    #include "linux/user.h"
}

#define RUFUSL_VERSION "0.1.20 Alpha"
#define MKFS_VERSION "4.0"

#define FS_FAT32 0
#define FS_NTFS 1

#define FS_FAT32_LABEL "FAT32"
#define FS_NTFS_LABEL "NTFS"

#define BS_512B 0
#define BS_1024B 1
#define BS_2048B 2
#define BS_4096B 3
#define BS_8192B 4
#define BS_16384B 5
#define BS_32768B 6

#define BS_512B_LABEL "512 bytes"
#define BS_1024B_LABEL "1024 bytes"
#define BS_2048B_LABEL "2048 bytes"
#define BS_4096B_LABEL "4096 bytes [Default]"
#define BS_8192B_LABEL "8192 bytes"
#define BS_16384B_LABEL "16384 bytes"
#define BS_32768B_LABEL "32768 bytes"

#define TB_MBR 0
#define TB_MBR_UEFI 1
#define TB_GPT 2

#define TB_MBR_LABEL "MBR partition table for BIOS/EFI Legacy"
#define TB_MBR_UEFI_LABEL "MBR partition table for EFI"
#define TB_GPT_LABEL "GPT Partition table for BIOS/UEFI"

#define SRC_ISO 0
#define SRC_DD 1

#define SRC_ISO_LABEL "ISO Image"
#define SRC_DD_LABEL "DD Image"

Log *logptr; /* Forward declaration */

RufusWindow::RufusWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::RufusWindow) {

  ui->setupUi(this);
  setupUi();
  scan();
  show();
  if (!check_root()) {
    r_printf("*** WARNING: Rufusl has been run as an unprivileged user.\n");
  } else {
      r_printf("*** rufusl: Running as root.\n");
  }
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

    this->worker = new RufusWorker();
    this->worker->start();

}

void RufusWindow::setProgress(int perc) {
    this->ui->progressBar->setValue(perc);
}

void RufusWindow::setupUi()
{
    /* Set up logger */

    this->log = new Log();

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
    this->ui->partitionCombo->addItem(TB_MBR_UEFI_LABEL);
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

  if (scan_devices(this->devices, MAX_DEVICES, &this->discovered) < 0) {
    this->ui->buttonStart->setEnabled(false);
  } else {
    this->ui->buttonStart->setEnabled(true);
  }


  /* Format device names and add to combo box */

  char buf[255];

  for (int i = 0; i < this->discovered; i++) {

    snprintf(buf, sizeof(buf), "%s %s (%s) [%.1lf GB]", devices[i].vendor,
             devices[i].model, devices[i].device, devices[i].capacity * 512 / 1000000000.0f);;

    this->box->addItem(buf);

    if (memset(buf, 0, sizeof(buf)) == NULL) return;

  }
}

RufusWindow::~RufusWindow() {
  delete box;
  delete log;
  delete ui;
}
