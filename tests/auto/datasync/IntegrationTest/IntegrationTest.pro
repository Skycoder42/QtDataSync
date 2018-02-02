include(../tests.pri)

TARGET = tst_integration

SOURCES += \
		tst_integration.cpp

BUILD_BIN_DIR = $$shadowed($$dirname(_QMAKE_CONF_))/bin
DEFINES += BUILD_BIN_DIR=\\\"$$BUILD_BIN_DIR/\\\"

!include(./setup.pri): SETUP_FILE = $$PWD/qdsapp.conf

DISTFILES += $$SETUP_FILE
DEFINES += SETUP_FILE=\\\"$$SETUP_FILE\\\"
