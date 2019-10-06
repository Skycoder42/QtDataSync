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

win32: QMAKE_SUBSTITUTES += qdsapp-install.bat.in
else:macos: QMAKE_SUBSTITUTES += de.skycoder42.qtdatasync.qdsapp.plist.in
else: QMAKE_SUBSTITUTES += qdsapp.service.in qdsapp.socket.in

DISTFILES += $$QMAKE_SUBSTITUTES \
	qdsapp.conf \
	docker_setup.conf \
	docker-compose.yaml \
	../../Dockerfile \
	dockerbuild/*

include(../../src/messages/messages.pri)

osx:!debug_and_release {
	CONFIG(release, debug|release): QDEP_LINK_DEPENDS = ../../src/messages/release/messages.pro
	else:CONFIG(debug, debug|release): QDEP_LINK_DEPENDS = ../../src/messages/debug/messages.pro
} else: QDEP_LINK_DEPENDS += ../../src/messages

win32 {
	QMAKE_TARGET_PRODUCT = "Qt Datasync Server"
	QMAKE_TARGET_COMPANY = $$COMPANY
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = $${BUNDLE_PREFIX}.
	CONFIG -= c++1z #TODO remove later
}

load(qt_app)

# svc-files install
install_system_service {
	install_conf.files += $$PWD/qdsapp.conf
	unix: install_conf.path = /etc
	else: install_conf.path = $$[QT_INSTALL_BINS]
	INSTALLS += install_conf

	win32 {
		install_bat.files += $$OUT_PWD/qdsapp-install.bat
		install_bat.path = $$[QT_INSTALL_BINS]
		INSTALLS += install_bat
	} else:macos {
		install_plist.files += $$OUT_PWD/de.skycoder42.qtdatasync.qdsapp.plist
		install_plist.path = /Library/LaunchDaemons
		INSTALLS += install_plist
	} else {
		install_svc.files += $$OUT_PWD/qdsapp.service $$OUT_PWD/qdsapp.socket
		install_svc.path = /usr/lib/systemd/system/
		INSTALLS += install_svc
	}
} else {
	install_svcconf.path = $$[QT_INSTALL_DATA]/services/$$APPNAME
	install_svcconf.files += $$PWD/qdsapp.conf
	win32: install_svcconf.files += $$OUT_PWD/qdsapp-install.bat
	else:macos: install_svcconf.files += $$OUT_PWD/de.skycoder42.qtdatasync.qdsapp.plist
	else: install_svcconf.files += $$OUT_PWD/qdsapp.service $$OUT_PWD/qdsapp.socket
	INSTALLS += install_svcconf
}

!load(qdep):error("Failed to load qdep feature! Run 'qdep prfgen --qmake $$QMAKE_QMAKE' to create it.")
