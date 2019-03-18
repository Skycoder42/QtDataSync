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

win32:msvc:CONFIG(debug, debug|release): CONFIG += disable_testrun
include($$PWD/../../testrun.pri)
