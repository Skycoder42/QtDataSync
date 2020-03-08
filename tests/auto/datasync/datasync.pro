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
		EngineTest \
		RemoteConnectorTest \
		TableDataModelTest

	AuthenticatorTest.depends += TestLib
	RemoteConnectorTest.depends += TestLib AuthenticatorTest
	EngineDataModelTest.depends += TestLib RemoteConnectorTest
	TableDataModelTest.depends += TestLib RemoteConnectorTest DbWatcherTest CloudTransformerTest
	EngineTest.depends += TestLib EngineDataModelTest TableDataModelTest SetupTest
}

prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests

win32:!win32-g++ {
	SUBDIRS -= SetupTest
	EngineTest.depends -= SetupTest
}
