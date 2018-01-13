TARGET  = qkwallet

QT += datasync KWallet
QT -= gui

HEADERS += \
	kwalletkeystoreplugin.h \
	kwalletkeystore.h

SOURCES += \
	kwalletkeystoreplugin.cpp \
	kwalletkeystore.cpp

DISTFILES += kwallet.json

PLUGIN_TYPE = keystores
PLUGIN_EXTENDS = datasync
PLUGIN_CLASS_NAME = KWalletKeyStorePlugin
load(qt_plugin)
