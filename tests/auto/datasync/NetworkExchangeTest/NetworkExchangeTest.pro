#-------------------------------------------------
#
# Project created by QtCreator 2017-08-27T18:04:30
#
#-------------------------------------------------

QT       += testlib network datasync-private

QT       -= gui

include (../tests.pri)

TARGET = tst_networkexchange
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += \
		tst_networkexchange.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
