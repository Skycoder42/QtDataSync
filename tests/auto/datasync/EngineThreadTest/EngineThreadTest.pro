TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_enginethread

HEADERS +=

SOURCES += \
	tst_enginethread.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
DEFINES += $$envDefine(FIREBASE_PROJECT_ID)
DEFINES += $$envDefine(FIREBASE_API_KEY)

include($$PWD/../testlib.pri)
include($$PWD/../../testrun.pri)
