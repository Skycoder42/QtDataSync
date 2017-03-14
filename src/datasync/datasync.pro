TARGET = QtDataSync

QT = core jsonserializer sql websockets

HEADERS += \
	qdatasync_global.h \
	asyncdatastore.h \
	asyncdatastore_p.h \
	authenticator.h \
	cachingdatastore.h \
	datamerger.h \
	datamerger_p.h \
	defaults.h \
	defaults_p.h \
	exception.h \
	localstore.h \
	remoteconnector.h \
	setup.h \
	setup_p.h \
	stateholder.h \
	synccontroller.h \
	synccontroller_p.h \
	task.h \
	wsauthenticator.h \
	wsauthenticator_p.h \
	changecontroller_p.h \
	sqllocalstore_p.h \
	sqlstateholder_p.h \
	storageengine_p.h \
	wsremoteconnector_p.h

SOURCES += \
	asyncdatastore.cpp \
	authenticator.cpp \
	cachingdatastore.cpp \
	changecontroller.cpp \
	datamerger.cpp \
	defaults.cpp \
	exception.cpp \
	localstore.cpp \
	remoteconnector.cpp \
	setup.cpp \
	sqllocalstore.cpp \
	sqlstateholder.cpp \
	stateholder.cpp \
	storageengine.cpp \
	synccontroller.cpp \
	task.cpp \
	wsauthenticator.cpp \
	wsremoteconnector.cpp

OTHER_FILES += \
	engine.qmodel

load(qt_module)

win32 {
	QMAKE_TARGET_PRODUCT = "QtDataSync"
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "de.skycoder42."
}
