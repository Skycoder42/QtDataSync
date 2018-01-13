TARGET  = qmackeychain

QT += datasync-private
QT -= gui

HEADERS += \
	mackeychainkeystoreplugin.h \
	mackeychainkeystore.h

SOURCES += \
	mackeychainkeystoreplugin.cpp \
	mackeychainkeystore.cpp

DISTFILES += mackeychain.json

LIBS += -framework CoreFoundation -framework Security

PLUGIN_TYPE = keystores
PLUGIN_EXTENDS = datasync
PLUGIN_CLASS_NAME = MacKeychainKeyStorePlugin
load(qt_plugin)
