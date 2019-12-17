TEMPLATE = subdirs

SUBDIRS += \
	datasync \
	translations \
	firebase-server

QMAKE_EXTRA_TARGETS += run-tests
