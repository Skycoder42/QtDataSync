include(../tests.pri)
include(../../../../src/3rdparty/cryptopp/cryptopp.pri)

QT += datasync-private

TARGET = tst_messages

SOURCES += \
		tst_messages.cpp

INCLUDEPATH += ../../../../src/datasync/messages
