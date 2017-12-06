TARGET  = plain

QT += datasync-private
QT -= gui

HEADERS += \
		plainkeystoreplugin.h \
    plainkeystore.h

SOURCES += \
		plainkeystoreplugin.cpp \
    plainkeystore.cpp

DISTFILES += plain.json

PLUGIN_TYPE = keystores
PLUGIN_EXTENDS = datasync
PLUGIN_CLASS_NAME = PlainKeyStorePlugin
load(qt_plugin)
