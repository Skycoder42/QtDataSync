#-------------------------------------------------
#
# Project created by QtCreator 2017-03-17T20:48:34
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

include(../tests.pri)

TARGET = tst_localstore
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += tst_localstore.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
