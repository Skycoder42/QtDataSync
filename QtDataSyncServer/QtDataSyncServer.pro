TEMPLATE = app

QT += core websockets backgroundprocess sql concurrent
QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

TARGET = QtDataSyncServer
VERSION = 1.0.0

QMAKE_TARGET_COMPANY = "Skycoder42"
QMAKE_TARGET_PRODUCT = "QtDataSyncServer"
QMAKE_TARGET_DESCRIPTION = $$QMAKE_TARGET_PRODUCT
QMAKE_TARGET_COPYRIGHT = "Felix Barz"

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += "TARGET=\\\"$$TARGET\\\""
DEFINES += "VERSION=\\\"$$VERSION\\\""
DEFINES += "COMPANY=\"\\\"$$QMAKE_TARGET_COMPANY\\\"\""

HEADERS += \
	clientconnector.h \
	app.h \
	client.h \
	databasecontroller.h

SOURCES += \
	clientconnector.cpp \
	app.cpp \
	client.cpp \
	databasecontroller.cpp

DISTFILES += \
	setup.conf
