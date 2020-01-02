TEMPLATE = subdirs

SUBDIRS += \
	DbWatcherTest \
	StatemachineTest

prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests
