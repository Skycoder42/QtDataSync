TEMPLATE = subdirs

SUBDIRS += \
	TestLib \
	DbWatcherTest \
	CloudTransformerTest \
	TableMachineTest \
	EngineMachineTest

!no_firebase_tests {
	SUBDIRS += \
		AuthenticatorTest \
		RemoteConnectorTest

		AuthenticatorTest.depends += TestLib
		RemoteConnectorTest.depends += TestLib AuthenticatorTest
}

prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests
