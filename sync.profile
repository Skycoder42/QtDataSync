%modules = (
	"QtDataSync" => "$basedir/src/datasync",
);

%classnames = (
	"asyncdatastore.h" => "AsyncDataStore",
	"authenticator.h" => "Authenticator",
	"cachingdatastore.h" => "CachingDataStoreBase,CachingDataStore",
	"datamerger.h" => "DataMerger",
	"defaults.h" => "Defaults",
	"encryptor.h" => "Encryptor",
	"exceptions.h" => "SetupException,SetupExistsException,SetupLockedException,InvalidDataException,DataSyncException",
	"localstore.h" => "LocalStore",
	"remoteconnector.h" => "RemoteConnector",
	"setup.h" => "Setup",
	"stateholder.h" => "StateHolder",
	"synccontroller.h" => "SyncController",
	"task.h" => "Task,GenericTask,UpdateTask",
	"wsauthenticator.h" => "WsAuthenticator",
	"userdatanetworkexchange.h" => "UserInfo,UserDataNetworkExchange",
);
