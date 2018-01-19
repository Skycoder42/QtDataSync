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

