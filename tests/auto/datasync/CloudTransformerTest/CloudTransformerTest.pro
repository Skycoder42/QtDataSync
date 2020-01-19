TEMPLATE = app

QT = core testlib datasync

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_cloudtransformer

SOURCES += \
	tst_cloudtransformer.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include($$PWD/../../testrun.pri)
