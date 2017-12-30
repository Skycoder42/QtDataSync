include(../tests.pri)

QT       += datasync-private

TARGET = tst_synccontroller

SOURCES += \
		tst_synccontroller.cpp \
    testresolver.cpp

HEADERS += \
    testresolver.h
