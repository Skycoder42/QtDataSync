#-------------------------------------------------
#
# Project created by QtCreator 2017-03-18T15:13:14
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

include(../tests.pri)

TARGET = tst_changecontroller
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_changecontroller.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
