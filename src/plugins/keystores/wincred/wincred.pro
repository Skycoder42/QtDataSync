TARGET  = qwincred

QT += datasync
QT -= gui

HEADERS += \
	wincredkeystoreplugin.h \
	wincredkeystore.h

SOURCES += \
	wincredkeystoreplugin.cpp \
	wincredkeystore.cpp

DISTFILES += wincred.json

LIBS += -ladvapi32

PLUGIN_TYPE = keystores
PLUGIN_EXTENDS = datasync
PLUGIN_CLASS_NAME = WinCredKeyStorePlugin
load(qt_plugin)
