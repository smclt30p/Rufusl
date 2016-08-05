#-------------------------------------------------
#
# Project created by QtCreator 2016-08-01T23:36:00
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Rufusl
TEMPLATE = app

CONFIG += c++11 O3

QMAKE_CFLAGS_WARN_ON = -Wno-sign-compare

SOURCES += main.cpp\
        ui/rufuswindow.cpp \
    ui/log.cpp \
    ui/about.cpp \
    linux_devices/devices.c \
    ui/devicecombobox.cpp \
    dosfstools/mkfs.fat.c \
    dosfstools/device_info.c \
    dosfstools/blkdev/blkdev.c \
    rufusworker.cpp \
    dosfstools/blkdev/linux_version.c

HEADERS  += ui/rufuswindow.h \
    log.h \
    ui/about.h \
    linux_devices/devices.h \
    ui/devicecombobox.h \
    dosfstools/device_info.h \
    dosfstools/msdos_fs.h \
    dosfstools/blkdev/blkdev.h \
    rufusworker.h \
    dosfstools/blkdev/linux_version.h \
    dosfstools/mkfs.fat.h

FORMS    += ui/rufuswindow.ui \
    ui/log.ui \
    ui/about.ui
