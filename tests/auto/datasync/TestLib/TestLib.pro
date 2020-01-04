TEMPLATE = lib

QT = core datasync

CONFIG += staticlib

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
