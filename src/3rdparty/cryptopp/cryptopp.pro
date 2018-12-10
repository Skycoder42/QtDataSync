TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

TARGET = qtcryptopp

win32:!win32-g++ {
	QMAKE_CXXFLAGS += /arch:AVX2
	MODULE_DEFINES += CRYPTOPP_DISABLE_ASM #TODO reenable again later
} else {
	QMAKE_CXXFLAGS += -Wno-keyword-macro -Wno-unused-const-variable -Wno-unused-private-field

	!isEmpty(ANDROID_TARGET_ARCH) {
		message("Building for android arch $$ANDROID_TARGET_ARCH")
		INCLUDEPATH += $$(ANDROID_NDK_ROOT)/sources/android/cpufeatures
		SOURCES += $$(ANDROID_NDK_ROOT)/sources/android/cpufeatures/cpu-features.c

		equals(ANDROID_TARGET_ARCH, armeabi-v7a) {
			QMAKE_CXXFLAGS -= -mfpu=vfp
			QMAKE_CXXFLAGS += -mfpu=neon
		} else:equals(ANDROID_TARGET_ARCH, arm64-v8a) {
			# nothing to be done
		} else:equals(ANDROID_TARGET_ARCH, x86) {
			QMAKE_CXXFLAGS += -maes -mpclmul -msha -msse4.1 -msse4.2 -mssse3
			# Do this for Android builds for now as the NDK is broken
			MODULE_DEFINES += CRYPTOPP_DISABLE_ASM
			warning("Disabled x86 crypto ASM")
		}
	} else {
		# assume "normal" x86 arch
		message("Building for host x86 arch")
		QMAKE_CXXFLAGS += -maes -mpclmul -msha -msse4.1 -msse4.2 -mssse3
	}
}

# Input
HEADERS += $$files(src/*.h)

SOURCES += $$files(src/*.cpp)

SOURCES -= \
	src/bench1.cpp \
	src/bench2.cpp \
	src/datatest.cpp \
	src/dlltest.cpp \
	src/pch.cpp \
	src/regtest1.cpp \
	src/regtest2.cpp \
	src/regtest3.cpp \
	src/test.cpp \
	src/validat0.cpp \
	src/validat1.cpp \
	src/validat2.cpp \
	src/validat3.cpp \
	src/validat4.cpp \
	src/TestScripts/cryptest-coverity.cpp

DISTFILES += cryptopp.pri

load(qt_build_paths)

message($$MODULE_BASE_OUTDIR)
HEADER_INSTALL_DIR = "$$MODULE_BASE_OUTDIR/include/cryptopp"
header_copy_c.name = copy cryptopp header ${QMAKE_FILE_BASE}.h
header_copy_c.input = HEADERS
header_copy_c.variable_out = INCLUDE_INSTALL_HEADERS
header_copy_c.commands = $$QMAKE_CHK_DIR_EXISTS $$shell_path($$HEADER_INSTALL_DIR) || $$QMAKE_MKDIR $$shell_path($$HEADER_INSTALL_DIR) \
	$$escape_expand(\n\t)$(QINSTALL) ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
header_copy_c.output = $$HEADER_INSTALL_DIR/${QMAKE_FILE_BASE}$${first(QMAKE_EXT_H)}
header_copy_c.CONFIG += target_predeps explicit_dependencies no_dependencies no_link
header_copy_c.depends = ${QMAKE_FILE_IN}
QMAKE_EXTRA_COMPILERS += header_copy_c

MODULE_INCLUDEPATH += "$$MODULE_BASE_OUTDIR/include"

load(qt_helper_lib)

QMAKE_EXTRA_TARGETS += lrelease
