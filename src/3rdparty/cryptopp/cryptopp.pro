TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

load(qt_helper_lib)

!win32 {
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
			DEFINES += CRYPTOPP_DISABLE_ASM
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

system($$QMAKE_MKDIR $$shell_path($$MODULE_BASE_OUTDIR/include))
system($$QMAKE_MKDIR $$shell_path($$MODULE_BASE_OUTDIR/include/cryptopp))
for(hdr, HEADERS): system($$QMAKE_COPY_FILE $$shell_path($$PWD/$$hdr) $$shell_path($$MODULE_BASE_OUTDIR/include/cryptopp/$$basename(hdr)))

target.path = $$[QT_INSTALL_LIBS]
qtConfig(static): INSTALLS += target #TODO use qt provided install instead

QMAKE_EXTRA_TARGETS += lrelease
