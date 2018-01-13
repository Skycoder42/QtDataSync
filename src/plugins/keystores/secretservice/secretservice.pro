TARGET  = qsecretservice

QT += datasync
QT -= gui

CONFIG += link_pkgconfig
PKGCONFIG += libsecret-1

HEADERS += \
	libsecretwrapper.h \
	secretservicekeystore.h \
	secretservicekeystoreplugin.h

SOURCES += \
	libsecretwrapper.cpp \
	secretservicekeystore.cpp \
	secretservicekeystoreplugin.cpp

DISTFILES += \
	secretservice.json

PLUGIN_TYPE = keystores
PLUGIN_EXTENDS = datasync
PLUGIN_CLASS_NAME = GSecretKeyStorePlugin
load(qt_plugin)
