TEMPLATE = subdirs

SUBDIRS += \
	TestLib \
	AuthenticatorTest \
	RemoteConnectorTest \
	DbWatcherTest \
	TableMachineTest \
	EngineMachineTest

AuthenticatorTest.depends += TestLib
RemoteConnectorTest.depends += TestLib AuthenticatorTest

prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests
