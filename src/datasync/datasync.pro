TARGET = QtDataSync

QT = core jsonserializer websockets

HEADERS += qtdatasync_global.h

SOURCES +=

DISTFILES += \
	datasync.qmodel \
	network.pdf

load(qt_module)

win32 {
	QMAKE_TARGET_PRODUCT = "QtDataSync"
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "de.skycoder42."
}
