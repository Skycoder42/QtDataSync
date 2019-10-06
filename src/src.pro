TEMPLATE = subdirs

SUBDIRS += \
	datasync \
	plugins \
	imports \
	messages \
	3rdparty \
	translations

messages.depends += 3rdparty
datasync.depends += messages 3rdparty
plugins.depends += datasync
imports.depends += datasync

android {
	SUBDIRS += datasyncandroid
	!android-embedded: SUBDIRS += java

	datasyncandroid.depends += datasync java
	imports.depends += datasyncandroid
}

ios {
	SUBDIRS += datasyncios

	datasyncios.depends += datasync
	imports.depends += datasyncios
}

QMAKE_EXTRA_TARGETS += run-tests
