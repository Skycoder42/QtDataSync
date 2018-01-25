TEMPLATE = app

QT       += core gui widgets datasync

TARGET = Sample

HEADERS += \
		widget.h \
	sampledata.h \
	accountdialog.h \
	exchangedialog.h

SOURCES += \
		main.cpp \
		widget.cpp \
	sampledata.cpp \
	accountdialog.cpp \
	exchangedialog.cpp

FORMS += \
		widget.ui \
	accountdialog.ui \
	exchangedialog.ui

DISTFILES += docker-compose.yaml

include(../../3rdparty/modeltest.pri)

target.path = $$[QT_INSTALL_EXAMPLES]/datasync/$$TARGET
INSTALLS += target

#not found by linker?
unix:!mac {
	LIBS += -L$$OUT_PWD/../../../lib #required to make this the first place to search
	LIBS += -L$$[QT_INSTALL_LIBS] -licudata
	LIBS += -L$$[QT_INSTALL_LIBS] -licui18n
	LIBS += -L$$[QT_INSTALL_LIBS] -licuuc
}

#add lib dir to rpath
mac: QMAKE_LFLAGS += '-Wl,-rpath,\'$$OUT_PWD/../../../lib\''
