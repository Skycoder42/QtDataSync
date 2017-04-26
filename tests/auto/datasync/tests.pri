QT += datasync

HEADERS += \
	$$PWD/testdata.h \
	$$PWD/mocklocalstore.h \
	$$PWD/tst.h \
	$$PWD/mockstateholder.h \
	$$PWD/mockremoteconnector.h \
	$$PWD/mockdatamerger.h \
	$$PWD/testobject.h \
    $$PWD/mockencryptor.h

SOURCES += \
	$$PWD/testdata.cpp \
	$$PWD/mocklocalstore.cpp \
	$$PWD/tst.cpp \
	$$PWD/mockstateholder.cpp \
	$$PWD/mockremoteconnector.cpp \
	$$PWD/mockdatamerger.cpp \
	$$PWD/testobject.cpp \
    $$PWD/mockencryptor.cpp

INCLUDEPATH += $$PWD

DEFINES += QT_DEPRECATED_WARNINGS

mac: QMAKE_LFLAGS += '-Wl,-rpath,\'$$OUT_PWD/../../../../lib\''
