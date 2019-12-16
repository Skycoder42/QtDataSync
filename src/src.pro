TEMPLATE = subdirs

SUBDIRS += \
	datasync \
	firebase-server \
	translations

QMAKE_EXTRA_TARGETS += run-tests
