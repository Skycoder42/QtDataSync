TEMPLATE = subdirs

!cross_compile: SUBDIRS += appserver

QMAKE_EXTRA_TARGETS += run-tests
