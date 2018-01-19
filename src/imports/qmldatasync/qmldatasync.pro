QT = core datasync qml

TARGETPATH = de/skycoder42/QtDataSync
IMPORT_VERSION = 1.0

HEADERS +=  qmldatasync_plugin.h \
	qqmldatastore.h

SOURCES +=  qmldatasync_plugin.cpp \
	qqmldatastore.cpp

OTHER_FILES += qmldir

IMPORT_VERSION = 1.0

load(qml_plugin)
