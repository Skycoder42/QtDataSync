TARGET = QtDataSyncAndroid

QT = core service androidextras datasync datasync-private

HEADERS += \
	androidbackgroundservice.h \
	qtdatasyncandroid_global.h \
    androidbackgroundservice_p.h

SOURCES += \
	androidbackgroundservice.cpp

load(qt_module)
