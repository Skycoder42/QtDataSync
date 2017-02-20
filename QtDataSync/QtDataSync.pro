#-------------------------------------------------
#
# Project created by QtCreator 2017-02-07T17:59:33
#
#-------------------------------------------------
TEMPLATE = lib

QT       += network sql jsonserializer websockets
QT       -= gui

TARGET = Qt5DataSync
VERSION = 1.0.0

win32 {
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_PRODUCT = "QtDataSync"
	QMAKE_TARGET_DESCRIPTION = $$QMAKE_TARGET_PRODUCT
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"

	CONFIG += skip_target_version_ext
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "de.skycoder42."
	QMAKE_FRAMEWORK_BUNDLE_NAME = "QtDataSync"

	CONFIG += lib_bundle
	QMAKE_LFLAGS_SONAME = '-Wl,-install_name,@rpath/'
	QMAKE_LFLAGS += '-Wl,-rpath,\'@executable_path/../Frameworks\''
}

DEFINES += QTDATASYNC_LIBRARY
DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += qtdatasync_global.h \
	setup.h \
	asyncdatastore.h \
	setup_p.h \
	task.h \
	storageengine.h \
	asyncdatastore_p.h \
	exception.h \
	localstore.h \
	sqllocalstore.h \
	stateholder.h \
	sqlstateholder.h \
	authenticator.h \
	remoteconnector.h \
	wsremoteconnector.h \
	datamerger.h \
	changecontroller.h \
	datamerger_p.h \
	wsauthenticator.h \
	wsauthenticator_p.h \
	defaults.h \
	defaults_p.h \
	synccontroller.h \
	synccontroller_p.h \
	cachingdatastore.h

SOURCES += \
	setup.cpp \
	asyncdatastore.cpp \
	task.cpp \
	storageengine.cpp \
	exception.cpp \
	localstore.cpp \
	sqllocalstore.cpp \
	stateholder.cpp \
	sqlstateholder.cpp \
	authenticator.cpp \
	remoteconnector.cpp \
	wsremoteconnector.cpp \
	datamerger.cpp \
	changecontroller.cpp \
	wsauthenticator.cpp \
	defaults.cpp \
	synccontroller.cpp \
    cachingdatastore.cpp

unix {
	target.path = /usr/lib
	INSTALLS += target
}

DISTFILES += \
	engine.qmodel
