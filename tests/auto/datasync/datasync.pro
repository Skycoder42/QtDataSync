TEMPLATE = subdirs

SUBDIRS += \
	DbWatcherTest \
	RemoteConnectorTest \
#	StatemachineTest \
	TestLib

RemoteConnectorTest.depends += TestLib

prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests
