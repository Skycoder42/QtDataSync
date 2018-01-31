#include "migrationhelper.h"
#include "migrationhelper_p.h"
#include "defaults.h"
#include "localstore_p.h"
#include "defaults_p.h"

#include <QtCore/QThreadPool>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QLockFile>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace QtDataSync;

const QString MigrationHelper::DefaultOldStorageDir = QStringLiteral("./qtdatasync_localstore");

MigrationHelper::MigrationHelper(QObject *parent) :
	MigrationHelper(DefaultSetup, parent)
{}

MigrationHelper::MigrationHelper(const QString &setupName, QObject *parent) :
	QObject(parent),
	d(new MigrationHelperPrivate(setupName))
{}

MigrationHelper::~MigrationHelper() {}

void MigrationHelper::startMigration(const QString &storageDir, MigrationFlags flags)
{
	QThreadPool::globalInstance()->start(new MigrationRunnable {
											 d->defaults,
											 this,
											 storageDir,
											 flags
										 });
}

// ------------- PRIVATE IMPLEMENTATION -------------

MigrationHelperPrivate::MigrationHelperPrivate(const QString &setupName) :
	defaults(DefaultsPrivate::obtainDefaults(setupName))
{}



MigrationRunnable::MigrationRunnable(const Defaults &defaults, MigrationHelper *helper, const QString &oldDir, MigrationHelper::MigrationFlags flags) :
	_defaults(defaults),
	_helper(helper),
	_oldDir(oldDir),
	_flags(flags),
	_logger(nullptr),
	_progress(0)
{
	setAutoDelete(true);
}

#define QTDATASYNC_LOG _logger

#define cleanDb() do { \
	db.close(); \
	db = QSqlDatabase(); \
	QSqlDatabase::removeDatabase(dbName); \
} while(false)

#define dbError(errorSource) do { \
	logCritical().noquote() << "Database operation failed with error:" \
							<< errorSource.lastError().text(); \
	migrationDone(false); \
	cleanDb(); \
	return; \
} while(false)

void MigrationRunnable::run()
{
	//step 0: logging
	QObject scope;
	_logger = _defaults.createLogger("migration", &scope);

	// check if the old storage exists
	QDir storageDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	if(!storageDir.cd(_oldDir)) {
		logInfo().noquote() << "The directory to be migrated does not exist. Assuming migration has already been done. Directory:"
							<< storageDir.absolutePath();
		migrationDone(true);
		return;
	}

	//lock the setup
	QLockFile lockFile(storageDir.absoluteFilePath(QStringLiteral(".lock")));
	if(!lockFile.tryLock()) {
		qint64 pid;
		QString host, process;
		lockFile.getLockInfo(&pid, &host, &process);
		logCritical().noquote() << "The old storage directy is already locked by another process. Lock information:"
								<< "\n\tPID:" << pid
								<< "\n\tHost:" << host
								<< "\n\tApp-Name:" << process;
		migrationDone(false);
		return;
	}
	logDebug().noquote() << "Found and locked old storage directory:"
						 << storageDir.absolutePath();

	//open the database
	auto dbName = QStringLiteral("database_migration_") + _defaults.setupName();
	auto db = QSqlDatabase::addDatabase(QStringLiteral("SQLITE"), dbName);
	db.setDatabaseName(storageDir.absoluteFilePath(QStringLiteral("store.db")));
	if(!db.open())
		dbError(db);

	//count the number of things to migrate
	QSqlQuery countQuery(db);
	countQuery.prepare(QStringLiteral("SELECT Count(*) FROM DataIndex"));
	if(!countQuery.exec())
		dbError(countQuery);
	if(!countQuery.first())
		dbError(countQuery);
	auto migrationCount = countQuery.value(0).toInt();
	if(_flags.testFlag(MigrationHelper::MigrateRemoteConfig))
		migrationCount++;
	if(migrationCount == 0) {
		logInfo().noquote() << "Nothing found to be migrated";
		migrationDone(true);
		cleanDb();
		return;
	} else
		migrationPrepared(migrationCount);

	//first step: migrate settings
	if(_flags.testFlag(MigrationHelper::MigrateRemoteConfig)) {
		QObject innerScope;
		auto currentSettings = _defaults.createSettings(&innerScope); //no group -> global
		auto oldSettings = new QSettings(storageDir.absoluteFilePath(QStringLiteral("config.ini")),
										 QSettings::IniFormat,
										 &innerScope);

		//migrate key by key
		copyConf(oldSettings, QStringLiteral("RemoteConnector/remoteEnabled"),
				 currentSettings, QStringLiteral("connector/enabled"));
		copyConf(oldSettings, QStringLiteral("RemoteConnector/remoteUrl"),
				 currentSettings, QStringLiteral("connector/remote/url"));
//		copyConf(oldSettings, QStringLiteral("RemoteConnector/verifyPeer"),
//				 currentSettings, QStringLiteral("connector/remote/verifyPeer")); //TODO add?
		copyConf(oldSettings, QStringLiteral("RemoteConnector/sharedSecret"),
				 currentSettings, QStringLiteral("connector/remote/accessKey"));

		copyConf(oldSettings, QStringLiteral("NetworkExchange/name"),
				 currentSettings, QStringLiteral("connector/remote/accessKey"));

		//special: copy headers
		oldSettings->beginGroup(QStringLiteral("RemoteConnector/headers"));
		currentSettings->beginGroup(QStringLiteral("connector/deviceName"));
		foreach(auto key, oldSettings->childKeys()) {
			copyConf(oldSettings, key,
					 currentSettings, key);
		}
		currentSettings->endGroup();
		oldSettings->endGroup();

		currentSettings->sync();
		migrationProgress();
	}

	//next: migrate all the data step by step
	LocalStore store(_defaults, &scope);
	for(auto offset = 0; true; offset += 1000) {
		QSqlQuery entryQuery(db);
		entryQuery.prepare(QStringLiteral("SELECT DataIndex.Type, DataIndex.Key, File, Changed, SyncState.Type, SyncState.Key "
										  "FROM DataIndex "
										  "FULL OUTER JOIN SyncState "
										  "ON DataIndex.Type = SyncState.Type AND DataIndex.Key = SyncState.Key "
										  "ORDER BY DataIndex.Type, DataIndex.Key, SyncState.Type, SyncState.Key"
										  "LIMIT 1000 OFFSET ?"));
		entryQuery.addBindValue(offset);
		if(!entryQuery.exec())
			dbError(entryQuery);

		if(!entryQuery.first())
			break;
		do {
			//extract data
			auto type = entryQuery.value(0).toByteArray();
			QString key = entryQuery.value(1).toString();
			if(type.isEmpty()) {
				type = entryQuery.value(4).toByteArray();
				key = entryQuery.value(5).toString();
			}
			auto file = entryQuery.value(2).toString();
			auto state = entryQuery.value(3).toInt();

			ObjectKey objKey(type, key);
			switch (state) {
			case 0: //unchanged
				if(!_flags.testFlag(MigrationHelper::MigrateChanged)) //when not migrating changes assume changed
					state = 1;
				Q_FALLTHROUGH();
			case 1: //changed
			{
				//find the storage directory
				auto tName = QString::fromUtf8("store/_" + QByteArray(type).toHex());
				auto mDir = storageDir;
				if(!mDir.cd(tName)) {
					logWarning() << "Failed to find file of stored data. Skipping"
								 << objKey;
					continue; //scope is the do-while loop
				}

				//read the data file as json
				QFile inFile(mDir.absoluteFilePath(file));
				if(!inFile.open(QIODevice::ReadOnly)) {
					logWarning().noquote() << "Failed to open file of stored data. Skipping"
										   << objKey << "with file error:"
										   << inFile.errorString();
					continue; //scope is the do-while loop
				}
				auto data = QJsonDocument::fromBinaryData(inFile.readAll()).object();
				inFile.close();
				if(data.isEmpty()) {
					logWarning() << "Failed read file of stored data. Skipping"
								 << objKey;
					continue; //scope is the do-while loop
				}

				//store in the store...
				auto scope = store.startSync(objKey);
				LocalStore::ChangeType type;
				quint64 version;
				QString fileName;
				tie(type, version, fileName, std::ignore) = store.loadChangeInfo(scope);
				if(type != LocalStore::NoExists && !_flags.testFlag(MigrationHelper::MigrateOverwriteData)) {
					logDebug() << "Skipping" << objKey << "as it would overwrite existing data";
					continue;
				}
				store.storeChanged(scope, version + 1, fileName, data, state == 1, type);
				store.commitSync(scope);
				logDebug() << "Migrated dataset" << objKey << "from the old store to the new one";
				break;
			}
			case 2: //deleted
				if(_flags.testFlag(MigrationHelper::MigrateChanged)) {
					//only when migrating changes, store the delete change
					auto scope = store.startSync(objKey);
					LocalStore::ChangeType type;
					quint64 version;
					tie(type, version, std::ignore, std::ignore) = store.loadChangeInfo(scope);
					if(type != LocalStore::NoExists && !_flags.testFlag(MigrationHelper::MigrateOverwriteData)) {
						logDebug() << "Skipping" << objKey << "as it would overwrite existing data";
						continue;
					}
					store.storeDeleted(scope, version, true, type);
					store.commitSync(scope);
					logDebug() << "Migrated deleted dataset" << objKey << "from the old store to the new one";
					break;
				}
				break;
			default:
				Q_UNREACHABLE();
			}

			migrationProgress();
		} while(entryQuery.next());
	}

	//cleanup merge
	cleanDb();
	lockFile.unlock();

	//delete old stuff if wished
	if(_flags.testFlag(MigrationHelper::MigrateWithCleanup)) {
		if(!storageDir.removeRecursively())
			logWarning() << "Failed to remove storage directory. Unable to cleanup after migration";
	}

	//complete migrate
	logInfo() << "Migration successfully completed";
	migrationDone(true);
}

void MigrationRunnable::migrationPrepared(int count)
{
	QMetaObject::invokeMethod(_helper, "migrationPrepared",
							  Q_ARG(int, count));
}

void MigrationRunnable::migrationProgress()
{
	QMetaObject::invokeMethod(_helper, "migrationProgress",
							  Q_ARG(int, ++_progress));
}

void MigrationRunnable::migrationDone(bool ok)
{
	QMetaObject::invokeMethod(_helper, "migrationDone",
							  Q_ARG(bool, ok));
}

void MigrationRunnable::copyConf(QSettings *old, const QString &oldKey, QSettings *current, const QString &newKey) const
{
	if((_flags.testFlag(MigrationHelper::MigrateOverwriteConfig) || !current->contains(newKey)) &&
	   old->contains(oldKey)) {
		current->setValue(newKey, old->value(oldKey));
		logDebug().noquote() << "Migrated old settings" << QString(old->group() + QLatin1Char('/') + oldKey)
							 << "to new settings as" << QString(current->group() + QLatin1Char('/') + newKey);
	}
}
