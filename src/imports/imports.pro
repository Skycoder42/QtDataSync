TEMPLATE = subdirs

SUBDIRS += \
	qmldatasync

qmldatasync.CONFIG += no_lrelease_target

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
