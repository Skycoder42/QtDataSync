include(../tests.pri)
include(../../../../src/3rdparty/cryptopp/cryptopp.pri)

QT       += datasync-private

TARGET = tst_remoteconnector

SOURCES += \
		tst_remoteconnector.cpp \
	mockserver.cpp

INCLUDEPATH += ../../../../src/datasync/messages

HEADERS += \
	mockserver.h
