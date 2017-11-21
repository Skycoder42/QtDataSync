TEMPLATE = subdirs

win32:cross_compile: CONFIG += winrt
!android:!ios:!winrt {
	#SUBDIRS = qdatasyncserver
	#qdatasyncserver.CONFIG = host_build
}

docTarget.target = doxygen
QMAKE_EXTRA_TARGETS += docTarget
