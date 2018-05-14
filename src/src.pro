TEMPLATE = subdirs

SUBDIRS += \
	datasync \
	plugins \
	imports \
	messages

datasync.depends += messages
plugins.depends += datasync
imports.depends += datasync

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
