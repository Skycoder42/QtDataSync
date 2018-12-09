system_cryptopp:unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += libcrypto++
} else {
	CRYPTOPP_DIR = $$shadowed($$dirname(_QMAKE_CONF_))/lib

	win32:CONFIG(release, debug|release): LIBS_PRIVATE += -L$$CRYPTOPP_DIR/ -lcryptopp
	else:win32:CONFIG(debug, debug|release): LIBS_PRIVATE += -L$$CRYPTOPP_DIR/ -lcryptoppd
	else:darwin:CONFIG(debug, debug|release): LIBS_PRIVATE += -L$$CRYPTOPP_DIR/ -lcryptopp_debug
	else: LIBS_PRIVATE += -L$$CRYPTOPP_DIR/ -lcryptopp

	win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$CRYPTOPP_DIR/libcryptopp.a
	else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$CRYPTOPP_DIR/libcryptoppd.a
	else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$CRYPTOPP_DIR/cryptopp.lib
	else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$CRYPTOPP_DIR/cryptoppd.lib
	else:darwin:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$CRYPTOPP_DIR/libcryptopp_debug.a
	else: PRE_TARGETDEPS += $$CRYPTOPP_DIR/libcryptopp.a

	INCLUDEPATH += $$PWD/src
	DEPENDPATH += $$PWD/src

	win32: DEFINES += QTDATASYNC_OSRNG_OVERWRITE
}
