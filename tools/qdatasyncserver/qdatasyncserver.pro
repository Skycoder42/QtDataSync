TEMPLATE = app

QT += websockets backgroundprocess sql concurrent
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

load(qt_tool)
load(resources)

HEADERS += \
	clientconnector.h \
	app.h \
	client.h \
	databasecontroller.h \
    singletaskqueue.h

SOURCES += \
	clientconnector.cpp \
	app.cpp \
	client.cpp \
	databasecontroller.cpp \
    singletaskqueue.cpp

DISTFILES += \
	setup.conf \
	docker_setup.conf \
	docker-compose.yaml \
	dockerbuild/makedocker.sh \
	dockerbuild/Dockerfile \
	dockerbuild/env_start.sh \
	dockerbuild/install.sh \
	dockerbuild/qt-installer-script.qs

include(../../src/datasync/messages/messages.pri)
include(../../src/3rdparty/cryptopp/cryptopp.pri)
include(../../src/3rdparty/cryptoqq/cryptoqq.pri)

win32 {
	QMAKE_TARGET_PRODUCT = "Qt Datasync Server"
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

!ReleaseBuild:!DebugBuild:!system(qpmx -d $$shell_quote($$_PRO_FILE_PWD_) --qmake-run init $$QPMX_EXTRA_OPTIONS $$shell_quote($$QMAKE_QMAKE) $$shell_quote($$OUT_PWD)): error(qpmx initialization failed. Check the compilation log for details.)
else: include($$OUT_PWD/qpmx_generated.pri)
