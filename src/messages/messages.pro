TEMPLATE = lib

CONFIG += staticlib
win32|darwin: CONFIG += debug_and_release build_all

QT = core

DESTDIR = $$shadowed($$dirname(_QMAKE_CONF_))/lib
TARGET = $$qtLibraryTarget(qtdatasync-messages)

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

include(../3rdparty/cryptopp/cryptopp.pri)

# dummy target, translations are all done in datasync
QMAKE_EXTRA_TARGETS += lrelease

!ReleaseBuild:!DebugBuild:!system(qpmx -d $$shell_quote($$_PRO_FILE_PWD_) --qmake-run init $$QPMX_EXTRA_OPTIONS $$shell_quote($$QMAKE_QMAKE) $$shell_quote($$OUT_PWD)): error(qpmx initialization failed. Check the compilation log for details.)
else: include($$OUT_PWD/qpmx_generated.pri)
