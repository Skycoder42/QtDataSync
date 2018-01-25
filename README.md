# QtDataSync
A simple offline-first synchronisation framework, to synchronize data of Qt applications between devices.

[![Travis Build Status](https://travis-ci.org/Skycoder42/QtDataSync.svg?branch=master)](https://travis-ci.org/Skycoder42/QtDataSync)
[![Appveyor Build status](https://ci.appveyor.com/api/projects/status/ygrlpdc6p3dcnmk1/branch/master?svg=true)](https://ci.appveyor.com/project/Skycoder42/qtdatasync/branch/master)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3793032e97024b6ebec2f84ec2fd61ad)](https://www.codacy.com/app/Skycoder42/QtDataSync?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Skycoder42/QtDataSync&amp;utm_campaign=Badge_Grade)
[![AUR](https://img.shields.io/aur/version/qt5-datasync.svg)](https://aur.archlinux.org/packages/qt5-datasync/)

## Features
- Document-Store like access, using QObjects and Q_GADGET classes
	- Threadsafe Synchronous access with global caching optimizations
	- Change signals on data modifications
- Qt "model" class (`QAbstractListModel`) to view store data in item views
- Stable offline storage, to make shure apps work even without network
- Remote synchronisation between devices
	- Multi-user and multi-device system - data is synchronized per user on all his devices
	- Data is stored permanently on devices, and temporarily on the server
	- Data is end to end encrypted
- Private keys are stored in secure keystores
	- Uses system keystore where possible. Currently supported:
		- KWallet
		- Freedesktop Secret Service API (i.e. gnome-keyring)
		- macOs/iOs Keychain
		- Windows credentials store
		- Android Shared Preferences
		- (Unsecure) plain text keystore as fallback
	- Custom keystores can be implemented via a plugin mechanism
- Includes a websocket based server application as a backend
	- Operates with a PostgreSQL database
	- Includes a docker-compose file for the postgresql server
	- Is provided as docker image itself
- Controller classes to control and monitor the sync state and the account
	- Can be used in any thread
- Class to exchange user identities between devices in a local network
- Supports multi process mode (access and control data and engine from any process, with some restrictions)
- Provides convenient QML bindings

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
3. Download the compiled modules from the release page. **Note:** You will have to add the correct ones yourself and may need to adjust some paths to fit your installation! In addition to that, you will have to download the modules this one depends on as well. See Section "Requirements" below.
4. Build it yourself! **Note:** This requires all build an runtime dependencies to be available (See Section "Requirements" below). If you don't have/need cmake, you can ignore the related warnings. To automatically build and install to your Qt installation, run:
	- `qmake`
	- `make qmake_all`
	- `make` (If you want the tests/examples/etc. run `make all`)
	- Optional steps:
		- `make doxygen` to generate the documentation
		- `make lrelease` to generate the translations
	- `make install`

## Requirements
The library as a high level synchronisation backend has a few dependencies. They are listed below by categories. When installing via a package manager or with Qt Maintenancetool (prefered methods, see section "Download/Installation"" above) all these dependencies will be installed automatically.

- Official Qt Modules:
	- QtCore (qtbase)
	- QtSql (qtbase) with the qsqlite plugin
	- QtWebSockets
	- QtScxml
	- QtRemoteObjects (currently a TP module)
- Custom Qt modules:
	- [QJsonSerializer](https://github.com/Skycoder42/QJsonSerializer)
- Additional stuff for building it yourself:
	- perl
	- [CryptoPP library](https://www.cryptopp.com/) (Can also be linked dynamically)
	- [qpmx](https://github.com/Skycoder42/qpmx)
	- [qpm](https://github.com/Cutehacks/qpm)
	- Some of the plugins also have additional dependencies:
		- kwallet: [KWallet Framework](https://api.kde.org/frameworks/kwallet/html/)
		- secretservice: libsecret-1 via pkgconfig
	- For the appserver:
		- qpsql plugin for the QtSql module

To actually run the server, it needs to connect to a SQL Database. A little more specific: A PostgreSQL database. You can specify a different one, by setting a custom database driver in the configuration file, **but** the SQL used is PostgreSQL, so unless your DBS supports the same SQL flavor, you won't be able to get it running without modifications. You can host a PostgreSQL database using docker by running `docker-compose up -d` from `tools/qdatasyncserver`. This is further described in the example explaining how to set up the server.

### Docker
The `qdsappserver` is also available as docker-image, [`skycoder42/qdsappserver`](https://hub.docker.com/r/skycoder42/qdsappserver/). The versions are equal, i.e. QtDataSync version 4.0.0 will work with the server Version 4.0.0 as well. The server is somewhat backwards compatible. This is checked on runtime for any client that connects.

## Usage
The datasync library is provided as a Qt module. Thus, all you have to do is add the module, and then, in your project, add `QT += datasync` to your `.pro` file! Please note that when you deploy your application, you need both the library *and* the keystore plugins you intend to use.

To learn how to set up the library and use it, check the examples below.

### Example
The following examples descibe the different parts of the module, and how to set them up. For a full, simple example, check the `examples/datasync/Sample` app. The following examples show:

- Include QtDataSync into you app, local storage only
- Access the storage
- Setup the sync server using docker
- Enable synchronisation for the app
- Add a new user

#### Initialize QtDataSync
After adding datasync to your project, the next step is to setup datasync. Typically, you will only have one instance per application, but by adjusting the configuration, you create multiple instances. It's similar to how QSqlDatabase is managed. Please note that there is one important rule: Only one process at a time may access once instance. This is defined via the local storage directory that is used. If you want to use it in multiple processes, you need to use `Setup::createPassive` in all secondary processes. Please read the documentation for more details.

To setup datasync, simply use the Setup class:
```cpp
QtDataSync::Setup()
	//do any additional setup, i.e. ".setLocalDir(...)" to specify a different local storage directory
	.create();
```

And thats it! On a second thread, the storage will be initialized. You can now use the different stores or the controller to access the data.

#### Using DataStore
The data store allows you to access the store. Assuming your datatype looks like this. Please check [QJsonSerializer](https://github.com/Skycoder42/QJsonSerializer) for details on how a datatype can look. The important part here is, that data you want to insert **must** have a `USER` property. This property is used to identify a dataset, it's like it's primary key. Please note that only one property can be mark as user property. You can use any type you want, as long as it can be converted from and to QString via QVariant (This means `QVariant::fromType(myData).toString()`, as well as `QVariant(string).value<MyData>()` have to work as expected).
```cpp
struct Data
{
	Q_GADGET

	Q_PROPERTY(int key MEMBER int USER true)
	Q_PROPERTY(QString value MEMBER value)

public:
	Post();

	int key;
	QString value;
};
```

You can for example load all existing datasets for this type by using:
```cpp
auto store = new QtDataSync::DataStore(this); //If not specified, the store will use the "default setup"

//store an entry
store->save<Data>({42, "tree"});
//load all entries
foreach(Data d, store->loadAll<Data>()) {
	qDebug() << d.key << d.value;
});
```

#### Setting up the remote server (qdatasyncserver)
In order to actually synchronize data, you will need some kind of remote server. This is a tool thats part of this module. It operates on a PostgreSQL database to store data. The following steps show you how to configure and run the server application.

##### Setting up PostgreSQL
The server itself will perform the neccessary setups. All you need is a running PostgreSQL instance. Create an empty database on it, and specify credentials etc. in the configuration file. If you just want to try it out, the example contains a docker-compose file that will setup such a server for you `tools/appserver/docker-compose.yaml`. Simply cd to that folder and run `docker-compose up -d` to start the database.

##### Setting up qdatasyncserver
Once the database was started, you can run the server. It uses a configuration file for setup.
TODO write more

#### Adding the remote to your app to enable sync
The last step is to tell you application which remote to use. Assuming you are running the server with the `docker_setup.conf`, the following setup will connect the server.
```cpp
QtDataSync::Setup()
	.setRemoteConfiguration(QUrl("ws://localhost:4242")) //url to the server to connect to
	.create();
```

Once thats done, the library will automatically connect to the server and create an account. To synchronize changes, all you need to do now is to add a second device to your account.

##### Adding a second device to your account
TODO write

## Security
### Encryption Protocol
### Server

## Documentation
The documentation is available on [github pages](https://skycoder42.github.io/QtDataSync/). It was created using [doxygen](http://www.doxygen.org/). The HTML-documentation and Qt-Help files are shipped together with the module for both the custom repository and the package on the release page. Please note that doxygen docs do not perfectly integrate with QtCreator/QtAssistant.

## References
TODO

- https://github.com/frankosterfeld/qtkeychain
