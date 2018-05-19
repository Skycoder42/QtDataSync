TEMPLATE = subdirs

!android:!ios:!winrt: SUBDIRS = appserver

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
