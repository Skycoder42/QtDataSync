TEMPLATE = subdirs

SUBDIRS += \
	datasync \
	plugins \
	imports \
	messages
datasync.depends += messages
plugins.depends += datasync
imports.depends += datasync

android: SUBDIRS += datasyncandroid
datasyncandroid.depends += datasync

imports.CONFIG += no_lrelease_target
plugins.CONFIG += no_lrelease_target

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease run-tests
