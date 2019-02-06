TEMPLATE = lib
CONFIG += static simd
CONFIG -= qt

TARGET = qtcryptopp
VERSION = 8.0.0

# Input
HEADERS += \
	src/3way.h \
	src/adler32.h \
	src/adv_simd.h \
	src/aes.h \
	src/aes_armv4.h \
	src/algebra.h \
	src/algparam.h \
	src/arc4.h \
	src/argnames.h \
	src/aria.h \
	src/asn.h \
	src/authenc.h \
	src/base32.h \
	src/base64.h \
	src/basecode.h \
	src/bench.h \
	src/blake2.h \
	src/blowfish.h \
	src/blumshub.h \
	src/camellia.h \
	src/cast.h \
	src/cbcmac.h \
	src/ccm.h \
	src/chacha.h \
	src/cham.h \
	src/channels.h \
	src/cmac.h \
	src/config.h \
	src/cpu.h \
	src/crc.h \
	src/cryptlib.h \
	src/darn.h \
	src/default.h \
	src/des.h \
	src/dh.h \
	src/dh2.h \
	src/dll.h \
	src/dmac.h \
	src/donna.h \
	src/donna_32.h \
	src/donna_64.h \
	src/donna_sse.h \
	src/drbg.h \
	src/dsa.h \
	src/eax.h \
	src/ec2n.h \
	src/eccrypto.h \
	src/ecp.h \
	src/ecpoint.h \
	src/elgamal.h \
	src/emsa2.h \
	src/eprecomp.h \
	src/esign.h \
	src/factory.h \
	src/fhmqv.h \
	src/files.h \
	src/filters.h \
	src/fips140.h \
	src/fltrimpl.h \
	src/gcm.h \
	src/gf256.h \
	src/gf2_32.h \
	src/gf2n.h \
	src/gfpcrypt.h \
	src/gost.h \
	src/gzip.h \
	src/hashfwd.h \
	src/hc128.h \
	src/hc256.h \
	src/hex.h \
	src/hight.h \
	src/hkdf.h \
	src/hmac.h \
	src/hmqv.h \
	src/hrtimer.h \
	src/ida.h \
	src/idea.h \
	src/integer.h \
	src/iterhash.h \
	src/kalyna.h \
	src/keccak.h \
	src/keccakc.h \
	src/lea.h \
	src/lubyrack.h \
	src/luc.h \
	src/mars.h \
	src/md2.h \
	src/md4.h \
	src/md5.h \
	src/mdc.h \
	src/mersenne.h \
	src/misc.h \
	src/modarith.h \
	src/modes.h \
	src/modexppc.h \
	src/mqueue.h \
	src/mqv.h \
	src/naclite.h \
	src/nbtheory.h \
	src/nr.h \
	src/oaep.h \
	src/oids.h \
	src/osrng.h \
	src/ossig.h \
	src/padlkrng.h \
	src/panama.h \
	src/pch.h \
	src/pkcspad.h \
	src/poly1305.h \
	src/polynomi.h \
	src/ppc_simd.h \
	src/pssr.h \
	src/pubkey.h \
	src/pwdbased.h \
	src/queue.h \
	src/rabbit.h \
	src/rabin.h \
	src/randpool.h \
	src/rc2.h \
	src/rc5.h \
	src/rc6.h \
	src/rdrand.h \
	src/resource.h \
	src/rijndael.h \
	src/ripemd.h \
	src/rng.h \
	src/rsa.h \
	src/rw.h \
	src/safer.h \
	src/salsa.h \
	src/scrypt.h \
	src/seal.h \
	src/secblock.h \
	src/seckey.h \
	src/seed.h \
	src/serpent.h \
	src/serpentp.h \
	src/sha.h \
	src/sha3.h \
	src/shacal2.h \
	src/shark.h \
	src/simeck.h \
	src/simon.h \
	src/simple.h \
	src/siphash.h \
	src/skipjack.h \
	src/sm3.h \
	src/sm4.h \
	src/smartptr.h \
	src/sosemanuk.h \
	src/speck.h \
	src/square.h \
	src/stdcpp.h \
	src/strciphr.h \
	src/tea.h \
	src/threefish.h \
	src/tiger.h \
	src/trap.h \
	src/trunhash.h \
	src/ttmac.h \
	src/tweetnacl.h \
	src/twofish.h \
	src/validate.h \
	src/vmac.h \
	src/wake.h \
	src/whrlpool.h \
	src/words.h \
	src/xed25519.h \
	src/xtr.h \
	src/xtrcrypt.h \
	src/zdeflate.h \
	src/zinflate.h \
	src/zlib.h

SOURCES += \
	src/3way.cpp \
	src/adler32.cpp \
	src/algebra.cpp \
	src/algparam.cpp \
	src/arc4.cpp \
	src/aria.cpp \
	src/ariatab.cpp \
	src/asn.cpp \
	src/authenc.cpp \
	src/base32.cpp \
	src/base64.cpp \
	src/basecode.cpp \
	src/bfinit.cpp \
	src/blake2.cpp \
	src/blowfish.cpp \
	src/blumshub.cpp \
	src/camellia.cpp \
	src/cast.cpp \
	src/casts.cpp \
	src/cbcmac.cpp \
	src/ccm.cpp \
	src/chacha.cpp \
	src/cham.cpp \
	src/channels.cpp \
	src/cmac.cpp \
	src/cpu.cpp \
	src/crc.cpp \
	src/cryptlib.cpp \
	src/darn.cpp \
	src/default.cpp \
	src/des.cpp \
	src/dessp.cpp \
	src/dh.cpp \
	src/dh2.cpp \
	src/dll.cpp \
	src/donna_32.cpp \
	src/donna_64.cpp \
	src/dsa.cpp \
	src/eax.cpp \
	src/ec2n.cpp \
	src/eccrypto.cpp \
	src/ecp.cpp \
	src/elgamal.cpp \
	src/emsa2.cpp \
	src/eprecomp.cpp \
	src/esign.cpp \
	src/files.cpp \
	src/filters.cpp \
	src/fips140.cpp \
	src/gcm.cpp \
	src/gf256.cpp \
	src/gf2_32.cpp \
	src/gf2n.cpp \
	src/gfpcrypt.cpp \
	src/gost.cpp \
	src/gzip.cpp \
	src/hc128.cpp \
	src/hc256.cpp \
	src/hex.cpp \
	src/hight.cpp \
	src/hmac.cpp \
	src/hrtimer.cpp \
	src/ida.cpp \
	src/idea.cpp \
	src/integer.cpp \
	src/iterhash.cpp \
	src/kalyna.cpp \
	src/kalynatab.cpp \
	src/keccak.cpp \
	src/keccakc.cpp \
	src/lea.cpp \
	src/luc.cpp \
	src/mars.cpp \
	src/marss.cpp \
	src/md2.cpp \
	src/md4.cpp \
	src/md5.cpp \
	src/misc.cpp \
	src/modes.cpp \
	src/mqueue.cpp \
	src/mqv.cpp \
	src/nbtheory.cpp \
	src/oaep.cpp \
	src/osrng.cpp \
	src/padlkrng.cpp \
	src/panama.cpp \
	src/pkcspad.cpp \
	src/poly1305.cpp \
	src/polynomi.cpp \
	src/pssr.cpp \
	src/pubkey.cpp \
	src/queue.cpp \
	src/rabbit.cpp \
	src/rabin.cpp \
	src/randpool.cpp \
	src/rc2.cpp \
	src/rc5.cpp \
	src/rc6.cpp \
	src/rdrand.cpp \
	src/rdtables.cpp \
	src/rijndael.cpp \
	src/ripemd.cpp \
	src/rng.cpp \
	src/rsa.cpp \
	src/rw.cpp \
	src/safer.cpp \
	src/salsa.cpp \
	src/scrypt.cpp \
	src/seal.cpp \
	src/seed.cpp \
	src/serpent.cpp \
	src/sha.cpp \
	src/sha3.cpp \
	src/shacal2.cpp \
	src/shark.cpp \
	src/sharkbox.cpp \
	src/simeck.cpp \
	src/simon.cpp \
	src/simple.cpp \
	src/skipjack.cpp \
	src/sm3.cpp \
	src/sm4.cpp \
	src/sosemanuk.cpp \
	src/speck.cpp \
	src/square.cpp \
	src/squaretb.cpp \
	src/strciphr.cpp \
	src/tea.cpp \
	src/tftables.cpp \
	src/threefish.cpp \
	src/tiger.cpp \
	src/tigertab.cpp \
	src/ttmac.cpp \
	src/tweetnacl.cpp \
	src/twofish.cpp \
	src/vmac.cpp \
	src/wake.cpp \
	src/whrlpool.cpp \
	src/xed25519.cpp \
	src/xtr.cpp \
	src/xtrcrypt.cpp \
	src/zdeflate.cpp \
	src/zinflate.cpp \
	src/zlib.cpp

SSE2_SOURCES += \
	src/chacha_simd.cpp \
	src/donna_sse.cpp \
	src/sse_simd.cpp

SSSE3_SOURCES += \
	src/aria_simd.cpp \
	src/cham_simd.cpp \
	src/gcm_simd.cpp \
	src/lea_simd.cpp \
	src/simeck_simd.cpp \
	src/simon128_simd.cpp \
	src/speck128_simd.cpp

SSE4_1_SOURCES += \
	src/blake2s_simd.cpp \
	src/blake2b_simd.cpp \
	src/simon64_simd.cpp \
	src/speck64_simd.cpp

SSE4_2_SOURCES += \
	src/crc_simd.cpp

AVX2_SOURCES += \
	src/chacha_avx.cpp

NEON_SOURCES += \
	src/aria_simd.cpp \
	src/blake2s_simd.cpp \
	src/blake2b_simd.cpp \
	src/chacha_simd.cpp \
	src/lea_simd.cpp \
	src/neon_simd.cpp \
	src/simeck_simd.cpp \
	src/simon64_simd.cpp \
	src/simon128_simd.cpp \
	src/speck64_simd.cpp \
	src/speck128_simd.cpp

ARMABI_SOURCES +=  \
	src/crc_simd.cpp \
	src/gcm_simd.cpp \
	src/rijndael_simd.cpp \
	src/sha_simd.cpp \
	src/shacal2_simd.cpp

ssse3 {
	QMAKE_CFLAGS_AESNI += $$QMAKE_CFLAGS_SSSE3
	AESNI_SOURCES += src/sm4_simd.cpp
}

sse4_1 {
	QMAKE_CFLAGS_AESNI += $$QMAKE_CFLAGS_SSE4_1
	AESNI_SOURCES += src/rijndael_simd.cpp
}

sse4_2 {
	QMAKE_CFLAGS_SHANI += $$QMAKE_CFLAGS_SSE4_2
	SHANI_SOURCES += \
		src/sha_simd.cpp \
		src/shacal2_simd.cpp
}

win32 {
	CONFIG += masm

	MASM_SOURCES += \
		src/rdrand.asm

	MASM_x64_SOURCES += \
		src/x64masm.asm \
		src/x64dll.asm
} else:!isEmpty(ANDROID_TARGET_ARCH) {
	INCLUDEPATH += $$(ANDROID_NDK_ROOT)/sources/android/cpufeatures
	SOURCES += $$(ANDROID_NDK_ROOT)/sources/android/cpufeatures/cpu-features.c

	equals(ANDROID_TARGET_ARCH, arm64-v8a) {
		SOURCES += $$ARMABI_SOURCES
	} else:equals(ANDROID_TARGET_ARCH, armeabi-v7a) {
		SOURCES += $$ARMABI_SOURCES

		NEON_ASM += \
			src/aes_armv4.S

		# Qt does not enable neon for armv7a
		QMAKE_CFLAGS -= -mfpu=vfp
		QMAKE_CFLAGS += -mfpu=neon
		QMAKE_CXXFLAGS -= -mfpu=vfp
		QMAKE_CXXFLAGS += -mfpu=neon
		CONFIG += neon
	} else:equals(ANDROID_TARGET_ARCH, x86) {
		# Do this for Android builds for now as the NDK is broken
		warning("Disabled x86 crypto ASM")

		MODULE_DEFINES += CRYPTOPP_DISABLE_ASM
		CONFIG -= simd

		SOURCES +=  \
			src/sse_simd.cpp
	}
} else:unix {
	QMAKE_CFLAGS_SSSE3 += -mpclmul
	QMAKE_CXXFLAGS += -Wno-keyword-macro -Wno-unused-const-variable -Wno-unused-private-field

	ios {
		QMAKE_CFLAGS += -maes -mpclmul -msha -msse4.1 -msse4.2 -mssse3
		QMAKE_CXXFLAGS += -maes -mpclmul -msha -msse4.1 -msse4.2 -mssse3
	}
}

DISTFILES += cryptopp.pri

load(qt_build_paths)

HEADER_INSTALL_DIR = "$$MODULE_BASE_OUTDIR/include/cryptopp"
!ReleaseBuild|!DebugBuild {
	!mkpath($$HEADER_INSTALL_DIR):error("Failed to create cryptopp header dir: $$HEADER_INSTALL_DIR")
	for(hdr, HEADERS):!system($$QMAKE_QMAKE -install qinstall "$$PWD/$$hdr" "$$HEADER_INSTALL_DIR/$$basename(hdr)"):error("Failed to install header file: $$hdr")
}

MODULE_INCLUDEPATH += "$$MODULE_BASE_OUTDIR/include"

load(qt_helper_lib)

# DEBUG
load(qt_build_config)
message(QT_CPU_FEATURES = $$eval(QT_CPU_FEATURES.$$QT_ARCH))
message($$CONFIG)
