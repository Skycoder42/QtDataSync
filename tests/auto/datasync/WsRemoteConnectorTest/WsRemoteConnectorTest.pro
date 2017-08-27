#-------------------------------------------------
#
# Project created by QtCreator 2017-03-20T18:19:28
#
#-------------------------------------------------

QT       += testlib datasync-private

QT       -= gui

include(../tests.pri)

TARGET = tst_wsremoteconnector
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_wsremoteconnector.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
DEFINES += BUILDDIR=\\\"$$OUT_PWD/\\\"

HEADERS +=

DISTFILES += server-setup.conf

include(setup.pri)
DEFINES += SETUP_CONF=\\\"$$SETUP_FILE\\\"
