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
	TestRemoteConnector \
	TestMigrationHelper \
    TestEventCursor

include_server_tests {
	SUBDIRS += \
		TestAppServer \
		IntegrationTest

	IntegrationTest.depends += TestAppServer #ensure those two don't run in parallel
}

include_server_tests: message("Please run 'sudo docker-compose -f $$absolute_path(../../../tools/appserver/docker-compose.yaml) up -d' to start the services needed for server tests")

for(subdir, SUBDIRS):!equals(subdir, "TestLib"): $${subdir}.depends += TestLib

prepareRecursiveTarget(run-tests)
QMAKE_EXTRA_TARGETS += run-tests
