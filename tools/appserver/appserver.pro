TEMPLATE = app

QT += websockets sql concurrent service
QT -= gui

CONFIG += console
CONFIG -= app_bundle

APPNAME = qdsapp
unix: TARGET = $${APPNAME}d
else:win32: TARGET = $${APPNAME}svc
else: TARGET = $$APPNAME

VERSION = $$MODULE_VERSION
COMPANY = Skycoder42
BUNDLE_PREFIX = de.skycoder42

DEFINES += BUILD_QDATASYNCSERVER QT_BUILD_DATASYNC_LIB #is build as part of the lib regarding exports
DEFINES += "APPNAME=\\\"$$APPNAME\\\""
DEFINES += "VERSION=\\\"$$VERSION\\\""
DEFINES += "COMPANY=\\\"$$COMPANY\\\""
DEFINES += "BUNDLE_PREFIX=\\\"$$BUNDLE_PREFIX\\\""

HEADERS += \
	clientconnector.h \
	client.h \
	databasecontroller.h \
	singletaskqueue.h \
    datasyncservice.h

SOURCES += \
	clientconnector.cpp \
	client.cpp \
	databasecontroller.cpp \
	singletaskqueue.cpp \
    datasyncservice.cpp

DISTFILES += \
	docker_setup.conf \
	docker-compose.yaml \
	../../Dockerfile \
	dockerbuild/env_start.sh \
	dockerbuild/install.sh \
	qdsapp.service \
	qdsapp.socket \
	qdsapp.conf \
    de.skycoder42.qtdatasync.qdsapp.plist \
    qdsapp-install.bat

include(../../src/messages/messages.pri)

win32 {
	QMAKE_TARGET_PRODUCT = "Qt Datasync Server"
	QMAKE_TARGET_COMPANY = $$COMPANY
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = $${BUNDLE_PREFIX}.
	CONFIG -= c++1z #TODO remove later
}

#not found by linker?
unix:!mac {
	LIBS += -L$$[QT_INSTALL_LIBS] -licudata
	LIBS += -L$$[QT_INSTALL_LIBS] -licui18n
	LIBS += -L$$[QT_INSTALL_LIBS] -licuuc
}

load(qt_app)

!ReleaseBuild:!DebugBuild:!system(qpmx -d $$shell_quote($$_PRO_FILE_PWD_) --qmake-run init $$QPMX_EXTRA_OPTIONS $$shell_quote($$QMAKE_QMAKE) $$shell_quote($$OUT_PWD)): error(qpmx initialization failed. Check the compilation log for details.)
else: include($$OUT_PWD/qpmx_generated.pri)

