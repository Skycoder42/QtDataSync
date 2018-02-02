TEMPLATE = subdirs

SUBDIRS += \
	TestLib \ #must be compiled first
	TestLocalStore \
	TestDataStore \
	TestDataTypeStore \
	TestChangeController \
	TestCryptoController \
	TestSyncController \
	TestRoThreadedBackend \
	TestMessages \
	TestKeystorePlugins \
	TestRemoteConnector \
	TestMigrationHelper

CONFIG += ordered

include_server_tests: SUBDIRS += \
	TestAppServer \
	IntegrationTest
