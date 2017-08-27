TEMPLATE = subdirs

CONFIG += ordered
SUBDIRS +=  \
	TestLib \
	LocalStoreTest \
	StateHolderTest \
	SqlStoreTest \
	ChangeControllerTest \
	CachingDataStoreTest \
	SqlStateHolderTest \
	WsRemoteConnectorTest \
	SetupTest \
	TinyAesEncryptorTest \
    NetworkExchangeTest
