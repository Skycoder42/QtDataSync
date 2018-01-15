TEMPLATE = subdirs

SUBDIRS += \
	TestLib \
	TestLocalStore \
	TestDataStore \
	TestDataTypeStore \
	TestChangeController \
	TestCryptoController \
	TestSyncController \
	TestRoThreadedBackend \
	TestMessages \
	TestKeystorePlugins \
	TestRemoteConnector

include_server_tests: SUBDIRS +=  \
	TestAppServer
