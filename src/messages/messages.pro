TEMPLATE = lib

CONFIG += static

TARGET = qtdatasync_messages

DEFINES += QT_BUILD_DATASYNC_LIB #is build as part of the lib regarding exports

HEADERS += \
	message_p.h \
	identifymessage_p.h \
	registermessage_p.h \
	asymmetriccrypto_p.h \
	accountmessage_p.h \
	loginmessage_p.h \
	welcomemessage_p.h \
	errormessage_p.h \
	syncmessage_p.h \
	changemessage_p.h \
	changedmessage_p.h \
	devicesmessage_p.h \
	removemessage_p.h \
	accessmessage_p.h \
	proofmessage_p.h \
	grantmessage_p.h \
	devicechangemessage_p.h \
	macupdatemessage_p.h \
	keychangemessage_p.h \
	devicekeysmessage_p.h \
	newkeymessage_p.h

SOURCES += \
	message.cpp \
	identifymessage.cpp \
	registermessage.cpp \
	asymmetriccrypto.cpp \
	accountmessage.cpp \
	loginmessage.cpp \
	welcomemessage.cpp \
	errormessage.cpp \
	syncmessage.cpp \
	changemessage.cpp \
	changedmessage.cpp \
	devicesmessage.cpp \
	removemessage.cpp \
	accessmessage.cpp \
	proofmessage.cpp \
	grantmessage.cpp \
	devicechangemessage.cpp \
	macupdatemessage.cpp \
	keychangemessage.cpp \
	devicekeysmessage.cpp \
	newkeymessage.cpp

DISTFILES += \
	messages.pri

include(../3rdparty/cryptopp/cryptopp.pri)

MODULE_INCLUDEPATH += $$PWD

load(qt_helper_lib)
CONFIG += qt warning_clean
QT = core

QDEP_DEPENDS += Skycoder42/CryptoQQ@1.2.0
QDEP_EXPORTS += Skycoder42/CryptoQQ@1.2.0
CONFIG += qdep_no_link

!load(qdep):error("Failed to load qdep feature! Run 'qdep prfgen --qmake $$QMAKE_QMAKE' to create it.")
