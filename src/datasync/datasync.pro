TARGET = QtDataSync

QT = core jsonserializer sql websockets scxml remoteobjects

HEADERS += \
	qtdatasync_global.h \
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
	conflictresolver_p.h \
	accountmanager.h \
	accountmanager_p.h \
	userexchangemanager.h \
	userexchangemanager_p.h \
	emitteradapter_p.h \
	changeemitter_p.h \
	signal_private_connect_p.h \
    migrationhelper.h \
    migrationhelper_p.h \
    remoteconfig.h \
    remoteconfig_p.h

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
	syncmanager_p.cpp \
	accountmanager.cpp \
	accountmanager_p.cpp \
	userexchangemanager.cpp \
	emitteradapter.cpp \
	changeemitter.cpp \
    migrationhelper.cpp \
    remoteconfig.cpp

STATECHARTS += \
	connectorstatemachine.scxml

REPC_SOURCE += \
	syncmanager_p.rep \
	accountmanager_p.rep \
	changeemitter_p.rep

REPC_REPLICA += $$REPC_SOURCE

TRANSLATIONS += \
	translations/qtdatasync_de.ts \
	translations/qtdatasync_template.ts

DISTFILES += $$TRANSLATIONS

include(rothreadedbackend/rothreadedbackend.pri)
include(messages/messages.pri)
include(../3rdparty/cryptopp/cryptopp.pri)

MODULE_PLUGIN_TYPES = keystores

load(qt_module)

win32 {
	QMAKE_TARGET_PRODUCT = "QtDataSync"
	QMAKE_TARGET_COMPANY = "Skycoder42"
	QMAKE_TARGET_COPYRIGHT = "Felix Barz"
} else:mac {
	QMAKE_TARGET_BUNDLE_PREFIX = "de.skycoder42."
}

qpmx_ts_target.path = $$[QT_INSTALL_TRANSLATIONS]
qpmx_ts_target.depends += lrelease
INSTALLS += qpmx_ts_target

!ReleaseBuild:!DebugBuild:!system(qpmx -d $$shell_quote($$_PRO_FILE_PWD_) --qmake-run init $$QPMX_EXTRA_OPTIONS $$shell_quote($$QMAKE_QMAKE) $$shell_quote($$OUT_PWD)): error(qpmx initialization failed. Check the compilation log for details.)
else: include($$OUT_PWD/qpmx_generated.pri)

#replace template qm by ts
qpmx_ts_target.files -= $$OUT_PWD/$$QPMX_WORKINGDIR/qtdatasync_template.qm
qpmx_ts_target.files += translations/qtdatasync_template.ts
