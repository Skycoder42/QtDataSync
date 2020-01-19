TEMPLATE = app

QT = core testlib datasync

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_enginemachine

HEADERS += \
	enginedatamodel_p.h

SOURCES += \
	enginedatamodel.cpp \
	tst_enginemachine.cpp

STATECHARTS += \
	$$PWD/../../../../src/datasync/enginestatemachine_p.scxml

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include($$PWD/../../testrun.pri)
