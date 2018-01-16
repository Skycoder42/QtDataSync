include(../tests.pri)

TARGET = tst_cryptocontroller

SOURCES += \
		tst_cryptocontroller.cpp

DEFINES += PLUGIN_DIR=\\\"$$OUT_PWD/../../../../plugins/keystores/\\\"
