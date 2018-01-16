TEMPLATE = lib

QT += datasync-private testlib
!linux: CONFIG += static

linux: DESTDIR = $$shadowed($$dirname(_QMAKE_CONF_))/lib

HEADERS += \
	testlib.h \
	testdata.h \
	testobject.h \
	mockserver.h \
	mockconnection.h \
	mockclient.h

SOURCES += \
	testlib.cpp \
	testdata.cpp \
	testobject.cpp \
	mockserver.cpp \
	mockconnection.cpp \
	mockclient.cpp

INCLUDEPATH += ../../../../src/datasync/messages

verbose_tests: DEFINES += VERBOSE_TESTS

include(../../../../src/3rdparty/cryptopp/cryptopp.pri)
