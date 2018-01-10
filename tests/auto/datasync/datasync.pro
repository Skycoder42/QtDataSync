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
    TestKeystorePlugins

CONFIG += ordered
