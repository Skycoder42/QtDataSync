TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_statemachine

SOURCES += \
	tst_statemachine.cpp

STATECHARTS += \
	$$PWD/../../../../src/datasync/enginestatemachine.scxml

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include($$PWD/../../testrun.pri)