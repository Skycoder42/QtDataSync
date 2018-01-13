TARGET  = qandroid

QT += datasync androidextras
QT -= gui

HEADERS += \
	androidkeystoreplugin.h \
	androidkeystore.h

SOURCES += \
	androidkeystoreplugin.cpp \
	androidkeystore.cpp

DISTFILES += android.json

PLUGIN_TYPE = keystores
PLUGIN_EXTENDS = datasync
PLUGIN_CLASS_NAME = AndroidKeyStorePlugin
load(qt_plugin)
