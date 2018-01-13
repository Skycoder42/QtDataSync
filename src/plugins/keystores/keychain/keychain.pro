TARGET  = qkeychain

QT += datasync
QT -= gui

HEADERS += \
	keychainkeystoreplugin.h \
	keychainkeystore.h

SOURCES += \
	keychainkeystoreplugin.cpp

OBJECTIVE_SOURCES += \
	keychainkeystore.mm

DISTFILES += keychain.json

LIBS += -framework Foundation -framework Security

PLUGIN_TYPE = keystores
PLUGIN_EXTENDS = datasync
PLUGIN_CLASS_NAME = KeychainKeyStorePlugin
load(qt_plugin)
