TEMPLATE = lib

QT = core testlib datasync datasync-private

CONFIG += staticlib

debug_and_release {
	CONFIG(debug, debug|release):SUFFIX = /debug
	CONFIG(release, debug|release):SUFFIX = /release
}
INCLUDEPATH += \
	$$OUT_PWD/../../../../src/datasync/$$QRESTBUILDER_DIR$${SUFFIX}

HEADERS += \
	anonauth.h \
	testlib.h

SOURCES += \
	anonauth.cpp \
	testlib.cpp

runtarget.target = run-tests
!compat_test {
	win32: runtarget.depends += $(DESTDIR_TARGET)
	else: runtarget.depends += $(TARGET)
}
QMAKE_EXTRA_TARGETS += runtarget
