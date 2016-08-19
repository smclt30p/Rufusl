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

LIBS += -L/lib -lparted

SOURCES += main.cpp\
        ui/rufuswindow.cpp \
    ui/log.cpp \
    ui/about.cpp \
    ui/devicecombobox.cpp \
    rufusworker.cpp \
    linux/user.c \
    ui/errordialog.cpp \
    linux/devices.c \
    linux/mounting.c \
    linux/partition.c \
    linux/fat32.c \
    iso.c


HEADERS  += ui/rufuswindow.h \
    log.h \
    ui/about.h \
    ui/devicecombobox.h \
    rufusworker.h \
    linux/user.h \
    ui/errordialog.h \
    linux/devices.h \
    linux/mounting.h \
    linux/partition.h \
    linux/fat32.h \
    definitions.h \
    iso.h

FORMS    += ui/rufuswindow.ui \
    ui/log.ui \
    ui/about.ui \
    ui/errordialog.ui

DISTFILES +=
