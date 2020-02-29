TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_settings

HEADERS +=

SOURCES += \
	tst_settings.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include($$PWD/../testlib.pri)
include($$PWD/../../testrun.pri)
