TARGET = QtDataSync

QT = core restclient

HEADERS += \
	firebase/firebaseapibase_p.h \
	qtdatasync_global.h

SOURCES += \
	firebase/firebaseapibase.cpp

REST_API_FILES += \
	firebase/firebaseerrorelement.xml \
	firebase/firebaseerrorcontent.xml \
	firebase/firebaseerror.xml \
	firebase/auth/mailsignuprequest.xml \
	firebase/auth/mailsignupresponse.xml \
	firebase/auth/mailsigninresponse.xml \
	firebase/auth/refreshresponse.xml \
	firebase/auth/auth_api.xml \
	firebase/firestore/data.xml \
	firebase/firestore/firestore_api.xml

INCLUDEPATH += firebase

load(qt_module)

win32 {
	QMAKE_TARGET_PRODUCT = "QtDataSync"
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "com.skycoder42."
}
