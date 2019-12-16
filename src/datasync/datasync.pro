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
	googleoauthflow_p.h \
	qtdatasync_global.h \
	setup.h \
	setup_p.h

SOURCES += \
	authenticator.cpp \
	databasewatcher.cpp \
	engine.cpp \
	exception.cpp \
	firebase/firebaseapibase.cpp \
	googleoauthflow.cpp \
	setup.cpp

REST_API_FILES += \
	firebase/firebaseerrorelement.xml \
	firebase/firebaseerrorcontent.xml \
	firebase/firebaseerror.xml \
	firebase/auth/auth_api.xml \
	firebase/auth/deleterequest.xml \
	firebase/auth/refreshresponse.xml \
	firebase/auth/signinrequest.xml \
	firebase/auth/signinresponse.xml \
	firebase/firestore/data.xml \
	firebase/firestore/firestore_api.xml \
	firebase/firestore/firestore_data.xml \
	firebase/firestore/rawdata.xml

INCLUDEPATH += firebase

load(qt_module)

win32 {
	QMAKE_TARGET_PRODUCT = "QtDataSync"
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "com.skycoder42."
}
