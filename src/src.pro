TEMPLATE = subdirs

SUBDIRS += \
        3rdparty \
	datasync \
	datasynccrypto \
	imports \
	translations

cloud_functions: SUBDIRS += firebase-server

datasynccrypto.depends += datasync 3rdparty
imports.depends += datasync

QMAKE_EXTRA_TARGETS += run-tests
