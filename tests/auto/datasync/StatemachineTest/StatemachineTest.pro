TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_statemachine

SMPATH = $$OUT_PWD/../../../../src/datasync/$$QSCXMLC_DIR

INCLUDEPATH += $$SMPATH
HEADERS += $$SMPATH/enginestatemachine.h
SOURCES += $$SMPATH/enginestatemachine.cpp

SOURCES += \
	tst_statemachine.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include($$PWD/../../testrun.pri)
