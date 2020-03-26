TARGET = QtDataSyncCrypto

QT = core datasync core-private datasync-private
CONFIG += static
MODULE_CONFIG += $$CPP_VERSION

HEADERS += \
	keymanager.h \
	keymanager_p.h \
	qtdatasynccrypto_global.h

SOURCES += \
    keymanager.cpp

load(qt_module)

QMAKE_USE_PRIVATE += cryptopp

win32 {
	QMAKE_TARGET_PRODUCT = "$$TARGET"
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "de.skycoder42."
}
