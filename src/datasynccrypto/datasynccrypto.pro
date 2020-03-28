TARGET = QtDataSyncCrypto

QT = core datasync core-private datasync-private
!equals(QTDATASYNC_CRYPTOPP, dynamic): CONFIG += static
MODULE_CONFIG += $$CPP_VERSION

HEADERS += \
	keymanager.h \
	keymanager_p.h \
	qiodevicesink.h \
	qiodevicesource.h \
	qtdatasynccrypto_global.h \
	securebytearray.h \
	symmetriccryptocloudtransformer.h \
	symmetriccryptocloudtransformer_p.h

SOURCES += \
	keymanager.cpp \
	qiodevicesink.cpp \
	qiodevicesource.cpp \
	securebytearray.cpp \
	symmetriccryptocloudtransformer.cpp

load(qt_module)

!loadCryptopp():error(Failed to load crypto++)

win32 {
	QMAKE_TARGET_PRODUCT = "$$TARGET"
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "de.skycoder42."
}
