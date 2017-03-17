#-------------------------------------------------
#
# Project created by QtCreator 2017-03-17T23:29:51
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

include(../tests.pri)

TARGET = tst_stateholder
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += tst_stateholder.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
