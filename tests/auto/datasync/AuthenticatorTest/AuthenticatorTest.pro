TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_authenticator

SOURCES += \
	tst_authenticator.cpp

LOGGING_RULES = "$$LOGGING_RULES;qt.datasync.FirebaseAuthenticator.warning=false"

include($$PWD/../testlib.pri)
include($$PWD/../../testrun.pri)

DEFINES += SRCDIR=\\\"$$PWD/\\\"

debug_and_release {
	CONFIG(debug, debug|release):SUFFIX = /debug
	CONFIG(release, debug|release):SUFFIX = /release
}
INCLUDEPATH += \
	$$OUT_PWD/../../../../src/datasync/$$QRESTBUILDER_DIR$${SUFFIX}
