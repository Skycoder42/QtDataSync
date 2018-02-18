TEMPLATE = subdirs

SUBDIRS += \
	datasync

datasync.CONFIG += no_lrelease_target

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
