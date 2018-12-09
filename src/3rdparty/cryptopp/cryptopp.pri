system_cryptopp:unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += libcrypto++
} else {
	CRYPTOPP_LIB_DIR = $$shadowed($$dirname(_QMAKE_CONF_))/lib
	CRYPTOPP_INC_DIR = $$shadowed($$dirname(_QMAKE_CONF_))/include

	win32:CONFIG(release, debug|release): LIBS_PRIVATE += -L$$CRYPTOPP_LIB_DIR/ -lcryptopp
	else:win32:CONFIG(debug, debug|release): LIBS_PRIVATE += -L$$CRYPTOPP_LIB_DIR/ -lcryptoppd
	else:darwin:CONFIG(debug, debug|release): LIBS_PRIVATE += -L$$CRYPTOPP_LIB_DIR/ -lcryptopp_debug
	else: LIBS_PRIVATE += -L$$CRYPTOPP_LIB_DIR/ -lcryptopp

	win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$CRYPTOPP_LIB_DIR/libcryptopp.a
	else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$CRYPTOPP_LIB_DIR/libcryptoppd.a
	else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$CRYPTOPP_LIB_DIR/cryptopp.lib
	else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$CRYPTOPP_LIB_DIR/cryptoppd.lib
	else:darwin:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$CRYPTOPP_LIB_DIR/libcryptopp_debug.a
	else: PRE_TARGETDEPS += $$CRYPTOPP_LIB_DIR/libcryptopp.a

	INCLUDEPATH += $$CRYPTOPP_INC_DIR
	DEPENDPATH += $$PWD/src

	win32:!win32-g++: DEFINES += QTDATASYNC_OSRNG_OVERWRITE CRYPTOPP_DISABLE_ASM
	equals(ANDROID_TARGET_ARCH, x86): DEFINES += CRYPTOPP_DISABLE_ASM
}
