TARGET = QtDataSyncCrypto

QT = core datasync core-private datasync-private
MODULE_CONFIG += $$CPP_VERSION

HEADERS += \
	qtdatasynccrypto_global.h

SOURCES +=

load(qt_module)

win32 {
	QMAKE_TARGET_PRODUCT = "$$TARGET"
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "de.skycoder42."
}
