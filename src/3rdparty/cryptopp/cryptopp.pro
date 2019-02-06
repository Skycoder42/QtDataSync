TEMPLATE = lib
CONFIG += static simd
CONFIG -= qt

TARGET = qtcryptopp
VERSION = 8.0.0

# DEBUG
load(qt_build_config)
message(QT_CPU_FEATURES = $$eval(QT_CPU_FEATURES.$$QT_ARCH))
message($$CONFIG)

# Input
HEADERS += $$files(src/*.h)

SOURCES += $$files(src/*.cpp)

SSE2_SOURCES += \
	src/chacha_simd.cpp \
	src/donna_sse.cpp \
	src/sse_simd.cpp

win32 {
	MASM_SOURCES += \
		rdrand.asm	

	MASM_x64_SOURCES += \
		x64masm.asm \
		x64dll.asm
}

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

unix: QMAKE_CFLAGS_SSSE3 += -mpclmul

EXTRA_MAYBE_SOURCES += src/sm4_simd.cpp \
	src/rijndael_simd.cpp \
	src/sha_simd.cpp \
	src/shacal2_simd.cpp

SOURCES -= $$SSE2_SOURCES $$SSSE3_SOURCES $$SSE4_1_SOURCES $$SSE4_2_SOURCES $$AVX2_SOURCES $$AESNI_SOURCES $$SHANI_SOURCES $$NEON_SOURCES $$ARMABI_SOURCES $$EXTRA_MAYBE_SOURCES

!isEmpty(ANDROID_TARGET_ARCH) {
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
} else:ios {
	QMAKE_CFLAGS += -maes -mpclmul -msha -msse4.1 -msse4.2 -mssse3
	QMAKE_CXXFLAGS += -maes -mpclmul -msha -msse4.1 -msse4.2 -mssse3
}

SOURCES -= \
	src/bench1.cpp \
	src/bench2.cpp \
	src/bench3.cpp \
	src/datatest.cpp \
	src/dlltest.cpp \
	src/fipsalgt.cpp \
	src/fipstest.cpp \
	src/pch.cpp \
	src/ppc_power7.cpp \
	src/ppc_power8.cpp \
	src/ppc_power9.cpp \
	src/ppc_simd.cpp \
	src/regtest1.cpp \
	src/regtest2.cpp \
	src/regtest3.cpp \
	src/regtest4.cpp \
	src/test.cpp \
	src/validat0.cpp \
	src/validat1.cpp \
	src/validat2.cpp \
	src/validat3.cpp \
	src/validat4.cpp \
	src/validat5.cpp \
	src/validat6.cpp \
	src/validat7.cpp \
	src/validat8.cpp \
	src/validat9.cpp \
	src/validat10.cpp \
	src/TestScripts/cryptest-coverity.cpp

DISTFILES += cryptopp.pri

load(qt_build_paths)

HEADER_INSTALL_DIR = "$$MODULE_BASE_OUTDIR/include/cryptopp"
!ReleaseBuild|!DebugBuild {
	!mkpath($$HEADER_INSTALL_DIR):error("Failed to create cryptopp header dir: $$HEADER_INSTALL_DIR")
	for(hdr, HEADERS):!system($$QMAKE_QMAKE -install qinstall "$$PWD/$$hdr" "$$HEADER_INSTALL_DIR/$$basename(hdr)"):error("Failed to install header file: $$hdr")
}

MODULE_INCLUDEPATH += "$$MODULE_BASE_OUTDIR/include"

load(qt_helper_lib)
