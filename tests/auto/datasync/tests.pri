QT += datasync

HEADERS += \
	$$PWD/testdata.h \
    $$PWD/mocklocalstore.h \
    $$PWD/tst.h \
    $$PWD/mockstateholder.h

SOURCES += \
	$$PWD/testdata.cpp \
    $$PWD/mocklocalstore.cpp \
    $$PWD/tst.cpp \
    $$PWD/mockstateholder.cpp

INCLUDEPATH += $$PWD
