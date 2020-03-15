TEMPLATE = subdirs

SUBDIRS += \
	datasync \
	datasynccrypto \
	imports \
	translations

cloud_functions: SUBDIRS += firebase-server

datasynccrypto.depends += datasync
imports.depends += datasync

QMAKE_EXTRA_TARGETS += run-tests
