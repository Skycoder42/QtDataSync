TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_authenticator

debug_and_release {
	CONFIG(debug, debug|release):SUFFIX = /debug
	CONFIG(release, debug|release):SUFFIX = /release
}
INCLUDEPATH += \
	$$OUT_PWD/../../../../src/datasync/$$QRESTBUILDER_DIR$${SUFFIX}

SOURCES += \
	tst_authenticator.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
DEFINES += FIREBASE_API_KEY=\\\"$$(FIREBASE_API_KEY)\\\"

include($$PWD/../testlib.pri)
include($$PWD/../../testrun.pri)
