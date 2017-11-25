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
	exception.h \
	objectkey.h

SOURCES += \
	localstore.cpp \
	defaults.cpp \
	logger.cpp \
	setup.cpp \
	exception.cpp \
	qtdatasync_global.cpp \
	objectkey.cpp

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

!ReleaseBuild:!DebugBuild:!system(qpmx -d $$shell_quote($$_PRO_FILE_PWD_) --qmake-run init $$QPMX_EXTRA_OPTIONS $$shell_quote($$QMAKE_QMAKE) $$shell_quote($$OUT_PWD)): error(qpmx initialization failed. Check the compilation log for details.)
else: include($$OUT_PWD/qpmx_generated.pri)
