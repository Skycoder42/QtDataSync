include(../tests.pri)
include(../../../../src/3rdparty/cryptopp/cryptopp.pri)

TARGET = tst_cryptocontroller

SOURCES += \
		tst_cryptocontroller.cpp

DEFINES += PLUGIN_DIR=\\\"$$OUT_PWD/../../../../plugins/keystores/\\\"
