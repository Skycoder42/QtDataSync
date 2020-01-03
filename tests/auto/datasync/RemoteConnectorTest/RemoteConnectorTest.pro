TEMPLATE = app

QT = core testlib datasync datasync-private

CONFIG += console
CONFIG -= app_bundle

TARGET = tst_remoteconnector

debug_and_release {
	CONFIG(debug, debug|release):SUFFIX = /debug
	CONFIG(release, debug|release):SUFFIX = /release
}
INCLUDEPATH += \
	$$PWD/../../../../src/3rdparty/qntp/include \
	$$PWD/../../../../src/datasync/firebase \
	$$PWD/../../../../src/datasync/firebase/realtimedb \
	$$OUT_PWD/../../../../src/datasync$${SUFFIX}/$$QRESTBUILDER_DIR \
	$$OUT_PWD/../../../../src/datasync$${SUFFIX}/$$QSCXMLC_DIR \

SOURCES += \
	tst_remoteconnector.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

include($$PWD/../../testrun.pri)

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../TestLib/release/ -lTestLib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../TestLib/debug/ -lTestLib
else:unix: LIBS += -L$$OUT_PWD/../TestLib/ -lTestLib

INCLUDEPATH += $$PWD/../TestLib
DEPENDPATH += $$PWD/../TestLib

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../TestLib/release/libTestLib.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../TestLib/debug/libTestLib.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../TestLib/release/TestLib.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../TestLib/debug/TestLib.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../TestLib/libTestLib.a
