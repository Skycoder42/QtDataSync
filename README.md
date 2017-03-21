# QtDataSync
A simple offline-first synchronisation framework, to synchronize data of Qt applications between devices.

## Features
- Document-Store like access, using QObjects and Q_GADGET classes
  - Asynchronous access, general access to all data, with futures and onResult handler
  - Synchronous access with caching store per datatype
- Stable offline storage, to make shure apps work even without network
- Remote synchronisation between devices
  - Multi-user and multi-device system - data is synchronized per user on all his devices
- Includes server application as backend, websocket based
- Controller class to control and monitor sync state
- Works "out of the box", since all components are provided
  - Can be used a simple local store only
  - Includes a docker-compose file for a PostgreSQL server
- Highly extensible
  - Custom local storage - for the offline local storage (default is implemented using SQLite and file storage)
  - Custom sync control - handle priorities for conflicts and implement custom merge algorithms
  - Custom remote connector - to synchronize to any backend (default uses websockets and the included server application)
  
## Requirements
In order to use the library, you will have to install [QJsonSerializer](https://github.com/Skycoder42/QJsonSerializer). If you want to use the server, [QtBackgroundProcess](https://github.com/Skycoder42/QtBackgroundProcess) needs to be available as well. **Hint:** If you install the module using the repository method as described in "Download/Installation" (or install it from AUR), those dependencies will be resolved automatically.

To actually run the server, it needs to connect to a SQL Database. A little more specific: A PostgreSQL database. You can specify a different one, by setting a custom database driver in the configuration file, **but** the SQL used is PostgreSQL, so unless your DBS supports the same SQL flavor, you won't be able to get it running without modifications. You can host a PostgreSQL database using docker by running `docker-compose up -d` from `tools/qdatasyncserver`. This is further described in the example explaining how to set up the server.

## Download/Installation
1. **Arch-Linux only:** If you are building against your system Qt, you can use my AUR-Repository: [qt5-datasync](https://aur.archlinux.org/packages/qt5-datasync/)
2. Simply add my repository to your Qt MaintenanceTool (Image-based How-To here: [Add custom repository](https://github.com/Skycoder42/QtModules/blob/master/README.md#add-my-repositories-to-qt-maintenancetool)):
  1. Open the MaintenanceTool, located in your Qt install directory (e.g. `~/Qt/MaintenanceTool`)
  2. Select `Add or remove components` and click on the `Settings` button
  3. Go to `Repositories`, scroll to the bottom, select `User defined repositories` and press `Add`
  4. In the right column (selected by default), type:
	- On Linux: https://install.skycoder42.de/qtmodules/linux_x64
	- On Windows: https://install.skycoder42.de/qtmodules/windows_x86
	- On Mac: https://install.skycoder42.de/qtmodules/mac_x64
  5. Press `Ok`, make shure `Add or remove components` is still selected, and continue the install (`Next >`)
  6. A new entry appears under all supported Qt Versions (e.g. `Qt > Qt 5.8 > Skycoder42 Qt modules`)
  7. You can install either all of my modules, or select the one you need: `Qt Data Sync`
  8. Continue the setup and thats it! you can now use the module for all of your installed Kits for that Qt Version
3. Download the compiled modules from the release page. **Note:** You will have to add the correct ones yourself and may need to adjust some paths to fit your installation! In addition to that, you will have to download the modules this one depends on as well.
4. Build it yourself! **Note:** This requires perl to be installed, as well as all dependencies. If you don't have/need cmake, you can ignore the related warnings. To automatically build and install to your Qt installation, run:
  - `qmake`
  - `make qmake_all`
  - `make`
  - `make install`
  
## Usage
The datasync library is provided as a Qt module. Thus, all you have to do is add the module, and then, in your project, add `QT += datasync` to your `.pro` file!

To learn how to set up the library and use it, check the examples below.

### Example
The following examples descibe the different parts of the module, and how to set them up. For a full, simple example, check the `examples/datasync/Example` app. The following examples show:

- Include QtDataSync into you app, local storage only
- Access the storage asynchronously
- Access the storage synchronously
- Setup the sync server using docker
- Enable synchronisation for the app

#### Initialize QtDataSync
After adding datasync to your project, the next step is to setup datasync. Typically, you will only have one instance per application, but by adjusting the configuration, you create multiple instances. It's similar to how QSqlDatabase is managed. Please note that there is one important rule: Only one process at a time may access once instance. This is defined via the local storage directory that is used. This means if you are using datasync and want to support multiple instances for your application, you will have to use different local storage directories per instance.

To setup datasync, simply use the Setup class:
```cpp
QtDataSync::Setup()
	//do any additional setup, i.e. ".setLocalDir(...)" to specify a different local storage directory
	.create(); //If you want multiple instances per app, you can set the setups name here
```

And thats it! On a second thread, the storage will be initialized. You can now use the different stores or the controller to access the data.

#### Using AsyncDataStore
The async data store allows you to access the store by using asynchronous operations, wrapped as tasks, The returned tasks extend QFuture, allowing you to use the just like a normal future. However, for GUI applications, waiting is not recommended, and thus the returned tasks have result handlers.

Assuming your datatype looks like this. Please check [QJsonSerializer](https://github.com/Skycoder42/QJsonSerializer) for details on how a datatype can look. The important part here is, that data you want to insert *must* have a `USER` property. This property is used to identify a dataset, it's like it's index. Please note that only one property can be mark as user property. You can use any type you want, as long as it can be converted from and to QString via QVariant (This means `QVariant::fromType(myData).toString()`, as well as `QVariant(string).value<MyData>()` have to work as expected).
```cpp
struct Data
{
	Q_GADGET

	Q_PROPERTY(int key MEMBER int USER true)
	Q_PROPERTY(QString value MEMBER value)

public:
	Post();

	int int;
	QString value;
};
```

You can for example load all existing datasets for this type by using:
```cpp
auto store = new QtDataSync::AsyncStore(this); //If not specified, the store will use the "default setup"

store->loadAll<Data>().onResult([](const QList<Data> &data){
	qDebug() << data.size();
});
```

#### Using CachingDataStore
While the async store is very useful for singular requests, If you need to "work" on the data, it can be hindering to always reload and wait for the asynchronous operations. The caching store is a wrapper around the async store, that caches **all entries** for one specific type, and automatically updates it's data if changes occur. The methods are the same, they simply return the result immediatly.

```cpp
auto store = new QtDataSync::CachingDataStore<Data, int> cacheStore(this);
//you cant use the store yet, because it needs to asynchronously load the data from the store once.
connect(store, &QtDataSync::CachingDataStoreBase::storeLoaded, this, [=](){
	//once the store has been loaded, you can use it:
	auto data = store->loadAll();
	qDebug() << data.size();
});
```

#### Setting up the remote server (qdatasyncserver)
In order to actually synchronize data, you will need some kind of remote server. The default implementation uses websockets, and connects to a `qdatasyncserver`. This is a tool thats part of this module. It operates on a PostgreSQL database to store data. The following steps show you how to configure and run the server application.

##### Setting up PostgreSQL
The server itself will perform the neccessary setups. All you need is a running PostgreSQL instance. Create an empty database on it, and specify credentials etc. in the configuration file. If you just want to try it out, the example contains a docker-compose file that will setup such a server for you `examples/datasync/Example/docker-compose.yaml`. Simply cd to that folder and run `docker-compose up -d` to start the database.

##### Setting up qdatasyncserver
Once the database was started, you can run the server. It uses a configuration file for setup. Run `qdatasyncserver --help` for all options, as well as the location on your machine. When starting, you can specify a custom config file as well. For all possible options, check `tools/qdatasyncserver/setup.conf`. If you started the database with the docker compose, you can use `tools/qdatasyncserver/docker_setup.conf` - It contains a working configuration for the docker database, and will launch the
qdatasyncserver on port 4242.

To actually start the server, use `qdatasyncserver start -c <path_to_config>`. This will start the server in the background and print out `ok` if starting worked. If it fails, check the logfile for details. You can find it's location by running `qdatasyncserver --help`. Look for the description of the `-L` option.

#### Adding the remote to your app to enable sync

### Extending/Using custom store elements

## Documentation
