TEMPLATE = app

QT += quick datasync datasyncios
CONFIG += c++14

TARGET = IosSync

HEADERS += \
	syncdelegate.h

SOURCES += \
	main.cpp \
	syncdelegate.cpp

RESOURCES += qml.qrc

QMAKE_INFO_PLIST = Info.plist

target.path = $$[QT_INSTALL_EXAMPLES]/datasync/$$TARGET
INSTALLS += target
