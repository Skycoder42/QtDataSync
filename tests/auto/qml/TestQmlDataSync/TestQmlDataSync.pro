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

DEFINES += SRCDIR=\\\"$$_PRO_FILE_PWD_/\\\"

DEFINES += KEYSTORE_PATH=\\\"$$OUT_PWD/../../../../plugins/keystores/\\\"
DEFINES += QML_PATH=\\\"$$OUT_PWD/../../../../qml/\\\"
