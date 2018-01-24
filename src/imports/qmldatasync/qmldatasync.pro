QT = core datasync qml

TARGETPATH = de/skycoder42/QtDataSync
IMPORT_VERSION = 1.0

HEADERS +=  qmldatasync_plugin.h \
	qqmldatastore.h \
	qqmlsyncmanager.h \
	qqmlaccountmanager.h \
	qqmldatastoremodel.h \
	qqmluserexchangemanager.h

SOURCES +=  qmldatasync_plugin.cpp \
	qqmldatastore.cpp \
	qqmlsyncmanager.cpp \
	qqmlaccountmanager.cpp \
	qqmldatastoremodel.cpp \
	qqmluserexchangemanager.cpp

OTHER_FILES += qmldir

IMPORT_VERSION = 1.0

generate_qmltypes {
	typeextra1.target = qmltypes
	typeextra1.depends += export LD_LIBRARY_PATH := "$$shadowed($$dirname(_QMAKE_CONF_))/lib/:$(LD_LIBRARY_PATH)"
	qmltypes.depends += typeextra1

	typeextra2.target = qmltypes
	typeextra2.depends += export QML2_IMPORT_PATH := "$$shadowed($$dirname(_QMAKE_CONF_))/qml/"
	qmltypes.depends += typeextra2
	QMAKE_EXTRA_TARGETS += typeextra1 typeextra2
}

load(qml_plugin)

generate_qmltypes {
	mfirst.target = all
	mfirst.depends += qmltypes
	QMAKE_EXTRA_TARGETS += mfirst
}
