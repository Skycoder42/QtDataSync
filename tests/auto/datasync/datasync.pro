TEMPLATE = subdirs

SUBDIRS += \
	StatemachineTest

prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests
