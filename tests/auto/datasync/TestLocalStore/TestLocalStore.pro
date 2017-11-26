TEMPLATE = app

QT       += testlib datasync-private
QT       -= gui

CONFIG   += console
CONFIG   -= app_bundle

TARGET = tst_localstore

include(../tests.pri)

SOURCES += \
		tst_localstore.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
