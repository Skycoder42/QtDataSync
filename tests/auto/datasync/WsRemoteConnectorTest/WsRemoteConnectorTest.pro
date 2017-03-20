#-------------------------------------------------
#
# Project created by QtCreator 2017-03-20T18:19:28
#
#-------------------------------------------------

QT       += testlib datasync-private

QT       -= gui

include(../tests.pri)

TARGET = tst_wsremoteconnector
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS


SOURCES += tst_wsremoteconnector.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS +=
