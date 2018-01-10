TEMPLATE = lib

QT += datasync testlib
CONFIG += static

HEADERS += \
	testlib.h \
	testdata.h \
	testobject.h

SOURCES += \
	testlib.cpp \
	testdata.cpp \
	testobject.cpp

verbose_tests: DEFINES += VERBOSE_TESTS
