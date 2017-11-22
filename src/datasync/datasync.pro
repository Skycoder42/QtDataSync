TARGET = QtDataSync

QT = core jsonserializer sql

HEADERS += qtdatasync_global.h \
	localstore_p.h \
	defaults_p.h \
	defaults.h \
    logger_p.h \
    logger.h \
    setup_p.h \
    setup.h \
    exceptions.h

SOURCES += \
	localstore.cpp \
	defaults.cpp \
    logger.cpp \
    setup.cpp \
    exceptions.cpp

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
