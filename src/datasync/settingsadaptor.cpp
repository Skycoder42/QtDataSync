#include "settingsadaptor_p.h"
#include "databasewatcher_p.h"
#include <QtCore/QRandomGenerator64>
#include <QtCore/QSaveFile>
using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logSettings, "qt.datasync.SettingsAdaptor")

const QString SettingsAdaptor::SettingsInfoConnectionKey = QStringLiteral("__qtdatasync_settings_info/connectionName");
const QString SettingsAdaptor::SettingsInfoTableKey = QStringLiteral("__qtdatasync_settings_info/tableName");

QSettings::Format SettingsAdaptor::registerFormat()
{
	static const auto cachedFormat = QSettings::registerFormat(QStringLiteral("qtdssl"),
															   &SettingsAdaptor::readSettings,
															   &SettingsAdaptor::writeSettings,
															   Qt::CaseSensitive);
	return cachedFormat;
}

bool SettingsAdaptor::createTable(QSqlDatabase database, const QString &table)
{
	if (!database.tables().contains(table)) {
		try {
			ExQuery createTableQuery{database, DatabaseWatcher::ErrorScope::Table, table};
			const auto escName = database.driver()->escapeIdentifier(table, QSqlDriver::TableName);
			createTableQuery.exec(QStringLiteral("CREATE TABLE %1 ( "
												 "	Key			TEXT NOT NULL, "
												 "	Version		INTEGER NOT NULL, "
												 "	Value		BLOB NOT NULL, "
												 "	PRIMARY KEY(Key) "
												 ");")
									  .arg(escName));
		} catch (SqlException &e) {
			qCCritical(logSettings) << e.what();
			return false;
		}
	}
	return true;
}

SettingsAdaptor::SettingsAdaptor(Engine *engine, QString &&table, QSqlDatabase database) :
	QObject{engine},
	_info{std::move(table), database.connectionName(), 0},
	_syncFile{new QTemporaryFile{QStringLiteral("qtds_settings_XXXXXX.qtdssl"), this}},
	_controller{engine->createController(_info.tableName, this)}
{
	_syncFile->open();
	_syncFile->close();

	connect(_controller, &TableSyncController::syncStateChanged,
			this, &SettingsAdaptor::syncStateChanged);
	_controller->start();

	connect(database.driver(), qOverload<const QString &>(&QSqlDriver::notification),
			this, &SettingsAdaptor::tableEvent);
	if (!database.driver()->subscribeToNotification(_info.tableName)) {
		qCWarning(logSettings) << "Failed to register notification hook for settings table" << _info.tableName
							   << "- sync will still work, but changes may not be detected in realtime";
	}

	updateSettings(); // force create settings file to make sure any QSettings instance will immediatly read
}

QString SettingsAdaptor::path() const
{
	return _syncFile->fileName();
}

void SettingsAdaptor::syncStateChanged(TableSyncController::SyncState state)
{
	switch (state) {
	case TableSyncController::SyncState::LiveSync:
	case TableSyncController::SyncState::Synchronized:
	case TableSyncController::SyncState::Error:
	case TableSyncController::SyncState::TemporaryError:
		// only trigger a settings reload on synchronized
		updateSettings();
		break;
	default:
		break;
	}
}

void SettingsAdaptor::tableEvent(const QString &name)
{
	if (name != _info.tableName)
		return;

	switch (_controller->syncState()) {
	case TableSyncController::SyncState::Downloading:
	case TableSyncController::SyncState::Uploading:
		// do nothing, as a sync event will be fired soon
		break;
	case TableSyncController::SyncState::LiveSync:
	case TableSyncController::SyncState::Synchronized:
	case TableSyncController::SyncState::Error:
	case TableSyncController::SyncState::TemporaryError:
		// in any active state, trigger a sync
		updateSettings();
		break;
	default:
		// in any other state, do nothing
		break;
	}
}

bool SettingsAdaptor::readSettings(QIODevice &device, QSettings::SettingsMap &map)
{
	const auto fDevice = qobject_cast<QFileDevice*>(&device);
	const auto devName = fDevice ? fDevice->fileName() : QStringLiteral("<unknown>");

	// read info
	QDataStream infoStream{&device};
	SettingsInfo info;
	infoStream.startTransaction();
	infoStream >> info;
	if (!infoStream.commitTransaction()) {
		qCCritical(logSettings) << "Unable to read settings information from file" << devName
								<< "with stream status" << infoStream.status()
								<< "and device error:" << qUtf8Printable(device.errorString());
		return false;
	}

	qCDebug(logSettings) << "Loading settings for table" << info.tableName
						 << "via connection" << info.connectionName
						 << "(seed:" << info.rnd << ")"
						 << "from file" << devName;

	// acquire (opened) database
	auto db = QSqlDatabase::database(info.connectionName, true);
	if (!db.isValid()) {
		qCCritical(logSettings) << "Unable to read settings - invalid SQL connection"
								<< info.connectionName;
		return false;
	}
	if (!db.isOpen()) {
		if (db.isOpenError()) {
			qCCritical(logSettings).noquote() << "Unable to read settings - failed to open SQL database with error:"
											  << db.lastError().text();
		} else {
			qCCritical(logSettings).noquote() << "Unable to read settings - SQL database not open";
		}
		return false;
	}

	const auto escName = db.driver()->escapeIdentifier(info.tableName, QSqlDriver::TableName);
	try {
		ExQuery loadDataQuery{db, DatabaseWatcher::ErrorScope::Table, info.tableName};
		loadDataQuery.exec(QStringLiteral("SELECT Key, Version, Value "
										  "FROM %1;")
							   .arg(escName));
		while (loadDataQuery.next()) {
			const auto data = loadDataQuery.value(2).toByteArray();
			QDataStream stream{data};
			stream.setVersion(loadDataQuery.value(1).toInt());
			QVariant var;
			stream.startTransaction();
			stream >> var;
			if (stream.commitTransaction())
				map.insert(loadDataQuery.value(0).toString(), var);
			else {
				qCWarning(logSettings) << "Failed to parse settings value for key"
									   << loadDataQuery.value(0).toString()
									   << "with datastream error:" << stream.status();
			}
		}

		map.insert(SettingsInfoConnectionKey, info.connectionName);
		map.insert(SettingsInfoTableKey, info.tableName);
		return true;
	} catch (SqlException &e) {
		qCCritical(logSettings) << e.what();
		return false;
	}
}

bool SettingsAdaptor::writeSettings(QIODevice &device, const QSettings::SettingsMap &map)
{
	const auto fDevice = qobject_cast<QFileDevice*>(&device);
	const auto devName = fDevice ? fDevice->fileName() : QStringLiteral("<unknown>");

	SettingsInfo info;
	// load connection name
	if (!loadValue(map, SettingsInfoConnectionKey, info.connectionName))
		return false;
	if (!loadValue(map, SettingsInfoTableKey, info.tableName))
		return false;

	qCDebug(logSettings) << "Storing settings for table" << info.tableName
						 << "via connection" << info.connectionName
						 << "into file" << devName;

	// acquire (opened) database
	auto db = QSqlDatabase::database(info.connectionName, true);
	if (!db.isValid()) {
		qCCritical(logSettings) << "Unable to write settings - invalid SQL connection"
								<< info.connectionName;
		return false;
	}
	if (!db.isOpen()) {
		if (db.isOpenError()) {
			qCCritical(logSettings).noquote() << "Unable to write settings - failed to open SQL database with error:"
											  << db.lastError().text();
		} else {
			qCCritical(logSettings).noquote() << "Unable to write settings - SQL database not open";
		}
		return false;
	}

	const auto escName = db.driver()->escapeIdentifier(info.tableName, QSqlDriver::TableName);
	try {
		ExTransaction transact{db, TransactionMode::Immeditate, info.tableName};

		ExQuery clearDataQuery{db, DatabaseWatcher::ErrorScope::Table, info.tableName};
		clearDataQuery.exec(QStringLiteral("DELETE FROM %1").arg(escName));

		for (auto it = map.begin(), end = map.end(); it != end; ++it) {
			// don't store table info keys
			if (it.key() == SettingsInfoConnectionKey ||
				it.key() == SettingsInfoTableKey)
				continue;

			QByteArray data;
			QDataStream stream{&data, QIODevice::WriteOnly};
			stream << *it;

			ExQuery storeDataQuery{db, DatabaseWatcher::ErrorScope::Entry, DatasetId{info.tableName, it.key()}};
			storeDataQuery.prepare(QStringLiteral("INSERT INTO %1 "
												  "(Key, Version, Value) "
												  "VALUES(?, ?, ?)")
									   .arg(escName));
			storeDataQuery.addBindValue(it.key());
			storeDataQuery.addBindValue(stream.version());
			storeDataQuery.addBindValue(data);
			storeDataQuery.exec();
		}

		transact.commit();
		return updateFile(&device, info, devName);
	} catch (SqlException &e) {
		qCCritical(logSettings) << e.what();
		return false;
	}
}

bool SettingsAdaptor::loadValue(const QSettings::SettingsMap &map, const QString &key, QString &target)
{
	const auto it = map.find(key);
	if (it != map.end()) {
		target = it->toString();
		return true;
	} else {
		qCCritical(logSettings).noquote() << "Datasync settings information was not found in the settings map"
										  << "(key" << key << "is missing)"
										  << "- cannot store settings!";
		return false;
	}
}

bool SettingsAdaptor::updateFile(QIODevice *device, SettingsInfo info, const QString &devName)
{
	info.rnd = QRandomGenerator64::global()->generate();
	qCDebug(logSettings) << "Writing settings with seed" << info.rnd
						 << "to file" << devName;
	QDataStream infoStream{device};
	infoStream << info;
	if (infoStream.status() == QDataStream::Ok)
		return true;
	else {
		qCCritical(logSettings) << "Failed to write settings information to file" << devName
								<< "with device error:" << qUtf8Printable(device->errorString());
		return false;
	}
}

void SettingsAdaptor::updateSettings()
{
	QSaveFile file{_syncFile->fileName()};
	if (!file.open(QIODevice::WriteOnly)) {
		qCWarning(logSettings) << "Failed to open settings file" << _syncFile->fileName()
							   << "to trigger an automatic settings reload with error"
							   << qUtf8Printable(file.errorString());
		return;
	}
	updateFile(&file, _info, _syncFile->fileName());
	if (!file.commit()) {
		qCWarning(logSettings) << "Failed to write settings file" << _syncFile->fileName()
							   << "to trigger an automatic settings reload with error"
							   << qUtf8Printable(file.errorString());
	}
}



QDataStream &operator<<(QDataStream &stream, const SettingsInfo &settings)
{
	stream << settings.connectionName
		   << settings.tableName
		   << settings.rnd;
	return stream;
}

QDataStream &operator>>(QDataStream &stream, SettingsInfo &settings)
{
	stream.startTransaction();
	stream >> settings.connectionName
		   >> settings.tableName
		   >> settings.rnd;
	stream.commitTransaction();
	return stream;
}
