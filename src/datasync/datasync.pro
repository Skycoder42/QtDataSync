TARGET = QtDataSync

QT = core restclient networkauth sql core-private

HEADERS += \
	authenticator.h \
	authenticator_p.h \
	databasewatcher_p.h \
	engine.h \
	engine_p.h \
	exception.h \
	firebase/firebaseapibase_p.h \
	firebase/realtimedb/servertimestamp_p.h \
	googleoauthflow_p.h \
	icloudtransformer.h \
	icloudtransformer_p.h \
	objectkey.h \
	qtdatasync_global.h \
	remoteconnector_p.h \
	setup.h \
	setup_p.h

SOURCES += \
	authenticator.cpp \
	databasewatcher.cpp \
	engine.cpp \
	exception.cpp \
	firebase/firebaseapibase.cpp \
	firebase/realtimedb/servertimestamp.cpp \
	googleoauthflow.cpp \
	icloudtransformer.cpp \
	objectkey.cpp \
	remoteconnector.cpp \
	setup.cpp

REST_API_FILES += \
	firebase/auth/auth_api.xml \
	firebase/auth/auth_error.xml \
	firebase/auth/deleterequest.xml \
	firebase/auth/errorelement.xml \
	firebase/auth/errorcontent.xml \
	firebase/auth/refreshresponse.xml \
	firebase/auth/signinrequest.xml \
	firebase/auth/signinresponse.xml \
	firebase/realtimedb/data.xml \
	firebase/realtimedb/realtimedb_api.xml \
	firebase/realtimedb/realtimedb_error.xml

INCLUDEPATH += \
	firebase \
	firebase/auth \
	firebase/realtimedb

!no_ntp {
	include(../3rdparty/qntp/qntp.pri)
	HEADERS += ntpsync_p.h
	SOURCES += ntpsync.cpp
} else {
	MODULE_DEFINES += QTDATASYNC_NO_NTP
	DEFINES += $$MODULE_DEFINES
}

load(qt_module)

win32 {
	QMAKE_TARGET_PRODUCT = "QtDataSync"
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "com.skycoder42."
}

DISTFILES += \
	syncflow.qmodel
