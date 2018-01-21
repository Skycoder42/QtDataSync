TEMPLATE = app
TARGET = tst_qmldatasync
CONFIG += qmltestcase
CONFIG += console
SOURCES += tst_qmldatasync.cpp

QT += datasync

importFiles.path = .
DEPLOYMENT += importFiles

DISTFILES += \
	tst_qmldatasync.qml

QML_IMPORT_PATH = $$OUT_PWD/../../../../qml/

DEFINES += SRCDIR=\\\"$$_PRO_FILE_PWD_/\\\"

DEFINES += KEYSTORE_PATH=\\\"$$OUT_PWD/../../../../plugins/keystores/\\\"
DEFINES += QML_PATH=\\\"$$QML_IMPORT_PATH\\\"

mac: QMAKE_LFLAGS += '-Wl,-rpath,\'$$OUT_PWD/../../../../lib\''
