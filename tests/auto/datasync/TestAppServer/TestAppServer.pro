TEMPLATE = app

QT = core testlib websockets sql concurrent #depend on everything appserver depends, to make shure all libraries are loaded

CONFIG   += console
CONFIG   -= app_bundle

DEFINES += SRCDIR=\\\"$$_PRO_FILE_PWD_/\\\"
BUILD_BIN_DIR = $$shadowed($$dirname(_QMAKE_CONF_))/bin
DEFINES += BUILD_BIN_DIR=\\\"$$BUILD_BIN_DIR/\\\"

mac: QMAKE_LFLAGS += '-Wl,-rpath,\'$$OUT_PWD/../../../../lib\''

TARGET = tst_appserver

SOURCES += \
		tst_appserver.cpp

!include(./setup.pri): SETUP_FILE = $$PWD/qdsapp.conf

DISTFILES += $$SETUP_FILE
DEFINES += SETUP_FILE=\\\"$$SETUP_FILE\\\"
