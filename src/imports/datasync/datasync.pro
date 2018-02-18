QT = core datasync qml

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

generate_qmltypes {
	typeextra1.target = qmltypes
	typeextra1.depends += export LD_LIBRARY_PATH := "$$shadowed($$dirname(_QMAKE_CONF_))/lib/:$(LD_LIBRARY_PATH)"
	typeextra2.target = qmltypes
	typeextra2.depends += export QML2_IMPORT_PATH := "$$shadowed($$dirname(_QMAKE_CONF_))/qml/"
	QMAKE_EXTRA_TARGETS += typeextra1 typeextra2
}

load(qml_plugin)

generate_qmltypes {
	qmltypes.depends = ../../../qml/$$TARGETPATH/$(TARGET)  #overwrite the target deps

	mfirst.target = all
	mfirst.depends += qmltypes
	QMAKE_EXTRA_TARGETS += mfirst
}
