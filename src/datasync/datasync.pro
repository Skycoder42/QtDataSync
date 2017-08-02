TARGET = QtDataSync

QT = core jsonserializer sql websockets
# uncomment the following line to keep data compability to previous versions
#QTAES_KEYSIZE = 128

include(../3rdparty/vendor/vendor.pri)

HEADERS += \
	asyncdatastore.h \
	asyncdatastore_p.h \
	authenticator.h \
	cachingdatastore.h \
	datamerger.h \
	datamerger_p.h \
	defaults.h \
	defaults_p.h \
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
	wsremoteconnector_p.h \
	exceptions.h \
	qtdatasync_global.h \
	encryptor.h \
	qtinyaesencryptor_p.h

SOURCES += \
	asyncdatastore.cpp \
	authenticator.cpp \
	cachingdatastore.cpp \
	changecontroller.cpp \
	datamerger.cpp \
	defaults.cpp \
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
	wsremoteconnector.cpp \
	exceptions.cpp \
	encryptor.cpp \
	qtinyaesencryptor.cpp

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
