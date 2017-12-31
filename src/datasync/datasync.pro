TARGET = QtDataSync

QT = core jsonserializer sql websockets scxml remoteobjects

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
	remoteconnector_p.h \
	cryptocontroller_p.h \
	keystore.h \
	controller_p.h \
	syncmanager_p.h \
	synchelper_p.h \
	synccontroller_p.h \
	conflictresolver.h \
	conflictresolver_p.h

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
	remoteconnector.cpp \
	cryptocontroller.cpp \
	keystore.cpp \
	controller.cpp \
	synchelper.cpp \
	synccontroller.cpp \
	conflictresolver.cpp \
    syncmanager_p.cpp

STATECHARTS += \
	connectorstatemachine.scxml

REPC_SOURCE += syncmanager_p.rep
REPC_REPLICA += $$REPC_SOURCE

# TODO add rep and statemachine headers to syncqt.pl

include(rothreadedbackend/rothreadedbackend.pri)
include(messages/messages.pri)
include(../3rdparty/cryptopp/cryptopp.pri)
include(../3rdparty/cryptoqq/cryptoqq.pri) #TODO qpmx

MODULE_PLUGIN_TYPES = keystores

load(qt_module)

win32 {
	QMAKE_TARGET_PRODUCT = "QtDataSync"
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "de.skycoder42."
}

# TODO add prl cleanup + ios ar merge

# IOS workaround until fixed
ios:exists(qpmx.ios.json):!system(rm qpmx.json && mv qpmx.ios.json qpmx.json):error(Failed to load temporary qpmx.json file)

!ReleaseBuild:!DebugBuild:!system(qpmx -d $$shell_quote($$_PRO_FILE_PWD_) --qmake-run init $$QPMX_EXTRA_OPTIONS $$shell_quote($$QMAKE_QMAKE) $$shell_quote($$OUT_PWD)): error(qpmx initialization failed. Check the compilation log for details.)
else: include($$OUT_PWD/qpmx_generated.pri)
