#-------------------------------------------------
#
# Project created by QtCreator 2017-03-25T17:55:58
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

include(../tests.pri)

TARGET = tst_setup
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_setup.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
