TARGET = QtDataSync

QT = core jsonserializer sql websockets

HEADERS += qtdatasync_global.h \
	localstore_p.h \
	defaults_p.h \
	defaults.h \
	logger_p.h \
	logger.h \
	setup_p.h \
	setup.h \
	exception.h \
	objectkey.h \
	datastore.h \
	datastore_p.h \
	qtdatasync_helpertypes.h \
	datatypestore.h \
	datastoremodel.h \
	datastoremodel_p.h \
	exchangeengine_p.h \
	syncmanager.h \
	changecontroller_p.h \
	remoteconnector_p.h

SOURCES += \
	localstore.cpp \
	defaults.cpp \
	logger.cpp \
	setup.cpp \
	exception.cpp \
	qtdatasync_global.cpp \
	objectkey.cpp \
	datastore.cpp \
	datatypestore.cpp \
	datastoremodel.cpp \
	exchangeengine.cpp \
	syncmanager.cpp \
	changecontroller.cpp \
	remoteconnector.cpp

DISTFILES += \
	datasync.qmodel \
	network.pdf \
	exchange.txt \
	network_connect.qmodel \
	network_exchange.qmodel \
	network_device.qmodel \
	network_keychange.qmodel

include(../3rdparty/cryptoqq/cryptoqq.pri)

system_cryptopp:unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += libcrypto++

	# debug
	INCLUDEPATH += $$PWD/../3rdparty/cryptopp/include
} else {
	win32:!win32-g++:CONFIG(release, debug|release): LIBS += -L$$PWD/../3rdparty/cryptopp/lib/ -lcryptlib
	else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS += -L$$PWD/../3rdparty/cryptopp/lib/ -lcryptlibd
	else: LIBS += -L$$PWD/../3rdparty/cryptopp/lib/ -lcryptopp

	INCLUDEPATH += $$PWD/../3rdparty/cryptopp/include
	DEPENDPATH += $$PWD/../3rdparty/cryptopp/include

	win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../3rdparty/cryptopp/lib/cryptlib.lib
	else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../3rdparty/cryptopp/lib/cryptlibd.lib
	else: PRE_TARGETDEPS += $$PWD/../3rdparty/cryptopp/lib/libcryptopp.a
}

load(qt_module)

win32 {
	QMAKE_TARGET_PRODUCT = "QtDataSync"
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "de.skycoder42."
}

# IOS workaround until fixed
ios:exists(qpmx.ios.json):!system(rm qpmx.json && mv qpmx.ios.json qpmx.json):error(Failed to load temporary qpmx.json file)

!ReleaseBuild:!DebugBuild:!system(qpmx -d $$shell_quote($$_PRO_FILE_PWD_) --qmake-run init $$QPMX_EXTRA_OPTIONS $$shell_quote($$QMAKE_QMAKE) $$shell_quote($$OUT_PWD)): error(qpmx initialization failed. Check the compilation log for details.)
else: include($$OUT_PWD/qpmx_generated.pri)
