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
android:!android-embedded: SUBDIRS += java
datasyncandroid.depends += datasync java

imports.CONFIG += no_lrelease_target
plugins.CONFIG += no_lrelease_target
datasyncandroid.CONFIG += no_lrelease_target
java.CONFIG += no_lrelease_target

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease run-tests
