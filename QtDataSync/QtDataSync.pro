#-------------------------------------------------
#
# Project created by QtCreator 2017-02-07T17:59:33
#
#-------------------------------------------------
TEMPLATE = lib

QT       += network sql jsonserializer
QT       -= gui

TARGET = Qt5DataSync

DEFINES += QTDATASYNC_LIBRARY
DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += qtdatasync_global.h \
	setup.h \
    asyncdatastore.h \
    setup_p.h \
    task.h \
    storageengine.h \
    asyncdatastore_p.h

SOURCES += \
	setup.cpp \
    asyncdatastore.cpp \
    task.cpp \
    storageengine.cpp

unix {
	target.path = /usr/lib
	INSTALLS += target
}
