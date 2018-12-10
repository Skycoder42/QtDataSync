system_cryptopp:unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += libcrypto++
} else {
	QMAKE_USE_PRIVATE += cryptopp
}
