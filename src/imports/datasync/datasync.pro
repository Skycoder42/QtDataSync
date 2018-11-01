QT = core datasync qml
android: QT += datasyncandroid

CXX_MODULE = datasync
TARGETPATH = de/skycoder42/QtDataSync
TARGET  = declarative_datasync
IMPORT_VERSION = $$MODULE_VERSION_IMPORT
DEFINES += "VERSION_MAJOR=$$MODULE_VERSION_MAJOR"
DEFINES += "VERSION_MINOR=$$MODULE_VERSION_MINOR"

HEADERS +=  \
	qqmldatastore.h \
	qqmlsyncmanager.h \
	qqmlaccountmanager.h \
	qqmldatastoremodel.h \
	qqmluserexchangemanager.h \
	qtdatasync_plugin.h

SOURCES +=  \
	qqmldatastore.cpp \
	qqmlsyncmanager.cpp \
	qqmlaccountmanager.cpp \
	qqmldatastoremodel.cpp \
	qqmluserexchangemanager.cpp \
	qtdatasync_plugin.cpp

OTHER_FILES += qmldir

CONFIG += qmlcache
load(qml_plugin)

generate_qmltypes {
	# run again to overwrite module env
	ldpath.name = LD_LIBRARY_PATH
	ldpath.value = "$$shadowed($$dirname(_QMAKE_CONF_))/lib/:$$[QT_INSTALL_LIBS]:$$(LD_LIBRARY_PATH)"
	qmlpath.name = QML2_IMPORT_PATH
	qmlpath.value = "$$shadowed($$dirname(_QMAKE_CONF_))/qml/:$$[QT_INSTALL_QML]:$$(QML2_IMPORT_PATH)"
	PLGDUMP_ENV = ldpath qmlpath
	QT_TOOL_ENV = ldpath qmlpath
	qtPrepareTool(QMLPLUGINDUMP, qmlplugindump)
	QT_TOOL_ENV =

	#overwrite the target deps as make target is otherwise not detected
	qmltypes.depends = ../../../qml/$$TARGETPATH/$(TARGET)
	OLDDMP = $$take_first(qmltypes.commands)
	qmltypes.commands = $$QMLPLUGINDUMP $${qmltypes.commands}
	message("replaced $$OLDDMP with $$QMLPLUGINDUMP")

	mfirst.target = all
	mfirst.depends += qmltypes
	QMAKE_EXTRA_TARGETS += mfirst
}

