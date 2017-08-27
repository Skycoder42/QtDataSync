TEMPLATE = app

QT       += core gui widgets datasync

TARGET = Example

HEADERS  += mainwidget.h \
	sampledata.h \
	setupdialog.h

SOURCES += main.cpp\
		mainwidget.cpp \
	sampledata.cpp \
	setupdialog.cpp

FORMS    += mainwidget.ui \
	setupdialog.ui

target.path = $$[QT_INSTALL_EXAMPLES]/datasync/Example
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
