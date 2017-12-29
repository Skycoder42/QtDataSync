include(../tests.pri)
include(../../../../src/3rdparty/cryptopp/cryptopp.pri)

QT       += datasync-private

TARGET = tst_cryptocontroller

SOURCES += \
		tst_cryptocontroller.cpp

INCLUDEPATH += ../../../../src/datasync/messages

DEFINES += PLUGIN_DIR=\\\"$$OUT_PWD/../../../../plugins/keystores/\\\"
