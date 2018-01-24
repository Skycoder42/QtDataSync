system_cryptopp:unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += libcrypto++

	# debug
	#INCLUDEPATH += $$PWD/include
} else {
	win32:!win32-g++:CONFIG(release, debug|release): LIBS_PRIVATE += -L$$PWD/lib/ -lcryptlib
	else:win32:!win32-g++:CONFIG(debug, debug|release): LIBS_PRIVATE += -L$$PWD/lib/ -lcryptlibd
	else: LIBS_PRIVATE += -L$$PWD/lib/ -lcryptopp

	INCLUDEPATH += $$PWD/include
	DEPENDPATH += $$PWD/include

	win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/cryptlib.lib
	else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/cryptlibd.lib
	else: PRE_TARGETDEPS += $$PWD/lib/libcryptopp.a

	win32: DEFINES += QTDATASYNC_OSRNG_OVERWRITE
}
