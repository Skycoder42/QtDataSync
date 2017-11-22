TEMPLATE = app

QT       += core gui widgets datasync

TARGET = Sample

HEADERS += \
		widget.h

SOURCES += \
		main.cpp \
		widget.cpp

FORMS += \
		widget.ui

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
