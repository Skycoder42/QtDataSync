TEMPLATE = subdirs

!android:!ios:!winrt {
	SUBDIRS = appserver
	qdatasyncserver.CONFIG = host_build
}

docTarget.target = doxygen
QMAKE_EXTRA_TARGETS += docTarget
