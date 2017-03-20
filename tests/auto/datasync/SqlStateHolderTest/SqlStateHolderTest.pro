#-------------------------------------------------
#
# Project created by QtCreator 2017-03-20T17:55:06
#
#-------------------------------------------------

QT       += testlib datasync-private

QT       -= gui

include(../tests.pri)

TARGET = tst_sqlstateholder
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += tst_sqlstateholder.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
