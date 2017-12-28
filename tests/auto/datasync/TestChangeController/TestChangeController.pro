include(../tests.pri)
include(../../../../src/3rdparty/cryptopp/cryptopp.pri)

QT += datasync-private

TARGET = tst_changecontroller

SOURCES += \
		tst_changecontroller.cpp

INCLUDEPATH += ../../../../src/datasync/messages
