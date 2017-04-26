#-------------------------------------------------
#
# Project created by QtCreator 2017-04-26T19:31:06
#
#-------------------------------------------------

QT       += testlib datasync-private

QT       -= gui

include (../tests.pri)

TARGET = tst_tinyaesencryptor
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += tst_tinyaesencryptor.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
