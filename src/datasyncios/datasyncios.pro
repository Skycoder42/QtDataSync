TARGET = QtDataSyncIos

QT = core datasync datasync-private
CONFIG += no_app_extension_api_only

HEADERS += \
	qtdatasyncios_global.h \
	qtdatasyncappdelegate_capi_p.h \
	QIOSApplicationDelegate+QtDatasyncAppDelegate_p.h \
    iossyncdelegate.h \
    iossyncdelegate_p.h

SOURCES += \
    iossyncdelegate.cpp

OBJECTIVE_SOURCES += \
	QIOSApplicationDelegate+QtDatasyncAppDelegate.mm

load(qt_module)
