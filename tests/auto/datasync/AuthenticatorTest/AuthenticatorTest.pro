TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_authenticator

SOURCES += \
	tst_authenticator.cpp

include($$PWD/../testlib.pri)
include($$PWD/../../testrun.pri)

DEFINES += SRCDIR=\\\"$$PWD/\\\"
