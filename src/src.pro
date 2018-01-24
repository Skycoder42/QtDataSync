TEMPLATE = subdirs

SUBDIRS += \
	datasync \
	plugins \
	imports

plugins.depends += datasync
imports.depends += datasync

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
