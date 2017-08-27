#-------------------------------------------------
#
# Project created by QtCreator 2017-03-18T18:50:09
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

include (../tests.pri)

TARGET = tst_cachingdatastore
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_cachingdatastore.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
