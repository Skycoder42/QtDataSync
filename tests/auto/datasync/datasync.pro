TEMPLATE = subdirs

SUBDIRS += \
	TestLib \
	TestLocalStore \
    TestDataStore \
    TestDataTypeStore \
    TestChangeController \
    TestCryptoController \
    TestSyncController

CONFIG += ordered
