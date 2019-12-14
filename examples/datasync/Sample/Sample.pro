TEMPLATE = app

QT += core gui quick datasync webview datasync-private

TARGET = Sample

SOURCES += \
	main.cpp

RESOURCES += qml.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/datasync/$$TARGET
!install_ok: INSTALLS += target
