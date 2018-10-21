TEMPLATE = subdirs

SUBDIRS += \
	TestQmlDataSync

prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests
