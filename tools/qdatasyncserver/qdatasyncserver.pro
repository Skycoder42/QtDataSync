TEMPLATE = app

QT += core websockets backgroundprocess sql concurrent
QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

TARGET = qdatasyncserver
VERSION = $$MODULE_VERSION
COMPANY = Skycoder42
BUNDLE_PREFIX = de.skycoder42

DEFINES += BUILD_QDATASYNCSERVER
DEFINES += "TARGET=\\\"$$TARGET\\\""
DEFINES += "VERSION=\\\"$$VERSION\\\""
DEFINES += "COMPANY=\\\"$$COMPANY\\\""
DEFINES += "BUNDLE_PREFIX=\\\"$$BUNDLE_PREFIX\\\""

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_ASCII_CAST_WARNINGS

load(qt_tool)
load(resources)

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
	setup.conf \
	docker_setup.conf \
	docker-compose.yaml \
	makedocker.sh \
	Dockerfile

win32 {
	QMAKE_TARGET_PRODUCT = "Qt Rest API Builder"
	QMAKE_TARGET_COMPANY = $$COMPANY
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = $${BUNDLE_PREFIX}.
}

#not found by linker?
unix:!mac {
	LIBS += -L$$[QT_INSTALL_LIBS] -licudata
	LIBS += -L$$[QT_INSTALL_LIBS] -licui18n
	LIBS += -L$$[QT_INSTALL_LIBS] -licuuc
}

dockerTarget.target = docker
dockerTarget.commands = $$PWD/makedocker.sh \"$$PWD\" \"$$[QT_INSTALL_LIBS]\" \"$$[QT_INSTALL_PLUGINS]\"
QMAKE_EXTRA_TARGETS += dockerTarget
