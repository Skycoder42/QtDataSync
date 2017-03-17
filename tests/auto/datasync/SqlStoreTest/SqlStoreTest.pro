#-------------------------------------------------
#
# Project created by QtCreator 2017-02-08T12:18:39
#
#-------------------------------------------------

QT       += testlib datasync-private

QT       -= gui

include(../tests.pri)

TARGET = tst_sqlstore
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += tst_sqlstore.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
