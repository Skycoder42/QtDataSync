TEMPLATE = lib

QT += datasync testlib
QT -= gui
CONFIG += staticlib

TARGET = TestLib

HEADERS += \
	mockdatamerger.h \
	mockencryptor.h \
	mocklocalstore.h \
	mockremoteconnector.h \
	mockstateholder.h \
	testdata.h \
	testobject.h \
	tst.h

SOURCES += \
	mockdatamerger.cpp \
	mockencryptor.cpp \
	mocklocalstore.cpp \
	mockremoteconnector.cpp \
	mockstateholder.cpp \
	testdata.cpp \
	testobject.cpp \
	tst.cpp
