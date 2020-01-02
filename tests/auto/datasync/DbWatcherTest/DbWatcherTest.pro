TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_dbwatcher

SOURCES += \
	tst_dbwatcher.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include($$PWD/../../testrun.pri)
