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

include_server_tests: SUBDIRS += \
	TestAppServer \
	IntegrationTest

for(subdir, SUBDIRS):!equals(subdir, "TestLib"): $${subdir}.depends += TestLib
