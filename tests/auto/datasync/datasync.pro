TEMPLATE = subdirs

SUBDIRS += \
	CloudTransformerTest \
	DbWatcherTest \
	EngineMachineTest \
	SettingsTest \
	SetupTest \
	TableMachineTest \
	TestLib

SettingsTest.depends += TestLib SetupTest

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
