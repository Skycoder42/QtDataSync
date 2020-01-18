TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

LOGGING_RULES = "$${LOGGING_RULES};qt.datasync.DatabaseProxy.warning=false;qt.datasync.Statemachine.Engine.debug=false"

TARGET = tst_remoteconnector

debug_and_release {
	CONFIG(debug, debug|release):SUFFIX = /debug
	CONFIG(release, debug|release):SUFFIX = /release
}
INCLUDEPATH += \
	$$PWD/../../../../src/3rdparty/qntp/include \
	$$PWD/../../../../src/datasync/firebase \
	$$PWD/../../../../src/datasync/firebase/realtimedb \
	$$OUT_PWD/../../../../src/datasync/$$QRESTBUILDER_DIR$${SUFFIX} \
	$$OUT_PWD/../../../../src/datasync/$$QSCXMLC_DIR$${SUFFIX} \

SOURCES += \
	tst_remoteconnector.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
DEFINES += FIREBASE_API_KEY=\\\"$$(FIREBASE_API_KEY)\\\"
DEFINES += FIREBASE_PROJECT_ID=\\\"$$(FIREBASE_PROJECT_ID)\\\"

include($$PWD/../testlib.pri)
include($$PWD/../../testrun.pri)
