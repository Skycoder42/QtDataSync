TEMPLATE = subdirs

!android:!ios:!winrt {
	SUBDIRS = appserver
	qdatasyncserver.CONFIG = host_build
}

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
