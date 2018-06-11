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

SVC_CONFIG_FILES = qdsapp.conf
win32: SVC_CONFIG_FILES += qdsapp-install.bat
else:macos: SVC_CONFIG_FILES += de.skycoder42.qtdatasync.qdsapp.plist
else: SVC_CONFIG_FILES += qdsapp.service qdsapp.socket

DISTFILES += $$SVC_CONFIG_FILES \
	docker_setup.conf \
	docker-compose.yaml \
	../../Dockerfile \
	dockerbuild/env_start.sh \
	dockerbuild/install.sh \

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
#unix:!mac {
#	LIBS += -L$$[QT_INSTALL_LIBS] -licudata
#	LIBS += -L$$[QT_INSTALL_LIBS] -licui18n
#	LIBS += -L$$[QT_INSTALL_LIBS] -licuuc
#}

load(qt_app)

# svc-files install
for(svcfile, SVC_CONFIG_FILES) {
	outfile = $$OUT_PWD/svcfiles/$$svcfile
	!exists($$outfile) {
		svcdata = $$cat($$svcfile, blob)
		svcdata = $$replace(svcdata, "%\\{QT_INSTALL_BINS\\}", "$$shell_path($$[QT_INSTALL_BINS])")
		!write_file($$outfile, svcdata): error(Failed to prepare service config file $$svcfile)
	}
	!install_system_service: install_svcconf.files += $$outfile
}
install_system_service {
	install_conf.files += $$OUT_PWD/svcfiles/qdsapp.conf
	unix: install_conf.path = /etc
	else: install_conf.path = $$[QT_INSTALL_BINS]
	INSTALLS += install_conf

	win32 {
		install_bat.files += $$OUT_PWD/svcfiles/qdsapp-install.bat
		install_bat.path = $$[QT_INSTALL_BINS]
		INSTALLS += install_bat
	} else:macos: {
		install_plist.files += $$OUT_PWD/svcfiles/de.skycoder42.qtdatasync.qdsapp.plist
		install_plist.path = /Library/LaunchDaemons
		INSTALLS += install_plist
	} else: {
		install_svc.files += $$OUT_PWD/svcfiles/qdsapp.service $$OUT_PWD/svcfiles/qdsapp.socket
		install_svc.path = /usr/lib/systemd/system/
		INSTALLS += install_svc
	}
} else {
	install_svcconf.path = $$[QT_INSTALL_DATA]/services/$$APPNAME
	INSTALLS += install_svcconf
}

!ReleaseBuild:!DebugBuild:!system(qpmx -d $$shell_quote($$_PRO_FILE_PWD_) --qmake-run init $$QPMX_EXTRA_OPTIONS $$shell_quote($$QMAKE_QMAKE) $$shell_quote($$OUT_PWD)): error(qpmx initialization failed. Check the compilation log for details.)
else: include($$OUT_PWD/qpmx_generated.pri)

