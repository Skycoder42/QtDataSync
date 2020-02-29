TEMPLATE = subdirs

SUBDIRS += \
	CloudTransformerTest \
	DbWatcherTest \
	EngineMachineTest \
	SetupTest \
	TableMachineTest \
	TestLib

!no_firebase_tests {
	SUBDIRS += \
		AuthenticatorTest \
		EngineDataModelTest \
		RemoteConnectorTest \
		TableDataModelTest

	AuthenticatorTest.depends += TestLib
	RemoteConnectorTest.depends += TestLib AuthenticatorTest
	EngineDataModelTest.depends += TestLib AuthenticatorTest RemoteConnectorTest
	TableDataModelTest.depends += TestLib AuthenticatorTest RemoteConnectorTest DbWatcherTest CloudTransformerTest
}

prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests
