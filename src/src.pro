TEMPLATE = subdirs

SUBDIRS += \
	datasync \
	imports \
	translations \
	firebase-server

imports.depends += datasync

QMAKE_EXTRA_TARGETS += run-tests
