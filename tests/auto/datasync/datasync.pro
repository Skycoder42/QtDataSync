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
		RemoteConnectorTest \
		EngineDataModelTest

	AuthenticatorTest.depends += TestLib
	RemoteConnectorTest.depends += TestLib AuthenticatorTest
	EngineDataModelTest.depends += TestLib AuthenticatorTest RemoteConnectorTest
}

prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests
