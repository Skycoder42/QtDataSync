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
  
## Usage
The datasync library is provided as a Qt module. Thus, all you have to do is add the module, and then, in your project, add `QT += datasync` to your `.pro` file!

To learn how to set up the library and use it, check the examples below.

### Example
The following examples descibe the different parts of the module, and how to set them up. For a full, simple example, check the `examples/Example` app. The following examples show:

- Include QtDataSync into you app, local storage only
- Access the storage asynchronously
- Access the storage synchronously
- Setup the sync server using docker
- Enable synchronisation for the app

#### A simple app with QtDataSync
#### Using AsyncDataStore
#### Using CachingDataStore
#### Setting up the remote server (qdatasyncserver)
#### Adding the remote to your app to enable sync

### Extending/Using custom store elements

## Download/Installation
## Documentation