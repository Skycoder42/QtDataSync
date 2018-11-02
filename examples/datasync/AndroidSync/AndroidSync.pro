TEMPLATE = app

QT += quick datasync datasyncandroid
CONFIG += c++14

TARGET = AndroidSync

HEADERS += \
	syncservice.h

SOURCES += \
	main.cpp \
	syncservice.cpp

RESOURCES += qml.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/datasync/$$TARGET
INSTALLS += target

DISTFILES += \
	android/AndroidManifest.xml \
	$$files(android/src/*, true) \
	$$files(android/res/*, true) \
	android/build.gradle

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
