TEMPLATE = subdirs

SUBDIRS += \
	datasync \
	imports \
	translations

cloud_functions: SUBDIRS += firebase-server

imports.depends += datasync

QMAKE_EXTRA_TARGETS += run-tests
