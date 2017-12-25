TARGET  = qgsecret

QT += datasync-private
QT -= gui

CONFIG += link_pkgconfig
PKGCONFIG += libsecret-1

HEADERS += \
	gsecretkeystoreplugin.h \
    gsecretkeystore.h \
    libsecretwrapper.h

SOURCES += \
	gsecretkeystoreplugin.cpp \
    gsecretkeystore.cpp \
    libsecretwrapper.cpp

DISTFILES += gsecret.json

PLUGIN_TYPE = keystores
PLUGIN_EXTENDS = datasync
PLUGIN_CLASS_NAME = GSecretKeyStorePlugin
load(qt_plugin)
