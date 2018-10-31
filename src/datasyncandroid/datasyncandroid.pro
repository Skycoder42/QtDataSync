TARGET = QtDataSyncAndroid

QT = core service androidextras datasync datasync-private

HEADERS += \
	androidbackgroundservice.h \
	qtdatasyncandroid_global.h \
    androidbackgroundservice_p.h \
    androidsynccontrol.h \
    androidsynccontrol_p.h

SOURCES += \
	androidbackgroundservice.cpp \
    androidsynccontrol.cpp

load(qt_module)
