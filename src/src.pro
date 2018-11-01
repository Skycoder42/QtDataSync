TEMPLATE = subdirs

SUBDIRS += \
	datasync \
	plugins \
	imports \
	messages

datasync.depends += messages
plugins.depends += datasync
imports.depends += datasync

imports.CONFIG += no_lrelease_target
plugins.CONFIG += no_lrelease_target

android {
	SUBDIRS += datasyncandroid
	!android-embedded: SUBDIRS += java

	datasyncandroid.depends += datasync java
	imports.depends += datasyncandroid

	datasyncandroid.CONFIG += no_lrelease_target
	java.CONFIG += no_lrelease_target
}

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease run-tests
