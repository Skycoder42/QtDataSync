TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_statemachine

STATECHARTS += \
	$$PWD/../../../../src/datasync/enginestatemachine.scxml

SOURCES += \
	tst_statemachine.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include($$PWD/../../testrun.pri)
