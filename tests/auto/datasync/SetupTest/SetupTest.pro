TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_setup

DS_SRC_DIR = $$PWD/../../../../src/datasync

SOURCES += \
	extensions.cpp \
	tst_setup.cpp

debug_and_release {
	CONFIG(debug, debug|release):SUFFIX = /debug
	CONFIG(release, debug|release):SUFFIX = /release
}
INCLUDEPATH += \
	$$DS_SRC_DIR \
	$$PWD/../../../../src/3rdparty/qntp/include \
	$$PWD/../../../../src/datasync/firebase \
	$$PWD/../../../../src/datasync/firebase/realtimedb \
	$$OUT_PWD/../../../../src/datasync \
	$$OUT_PWD/../../../../src/datasync/$$QRESTBUILDER_DIR$${SUFFIX} \
	$$OUT_PWD/../../../../src/datasync/$$QSCXMLC_DIR$${SUFFIX}
DEPENDPATH += $$DS_SRC_DIR

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include($$PWD/../../testrun.pri)

HEADERS += \
	extensions.h
