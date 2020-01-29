#include "settingsadaptor_p.h"
#include "databasewatcher_p.h"
using namespace QtDataSync;

Q_LOGGING_CATEGORY(QtDataSync::logSettings, "qt.datasync.SettingsAdaptor")

QSettings::Format SettingsAdaptor::registerFormat()
{
	return QSettings::registerFormat(QStringLiteral("qtdssl"),
									 &SettingsAdaptor::readSettings,
									 &SettingsAdaptor::writeSettings,
									 Qt::CaseSensitive);
}

void SettingsAdaptor::createTable(QSqlDatabase database, const QString &name)
{
	ExQuery createTableQuery{database, DatabaseWatcher::ErrorScope::Table, name};
	const auto escName = database.driver()->escapeIdentifier(name, QSqlDriver::TableName);
	createTableQuery.exec(QStringLiteral("CREATE TABLE %1 ( "
										 "	Key			TEXT NOT NULL, "
										 "	Version		INTEGER NOT NULL, "
										 "	Value		BLOB NOT NULL, "
										 "	PRIMARY KEY(Key) "
										 ");")
							  .arg(escName));
}

SettingsAdaptor::SettingsAdaptor(QSettings *settings) :
	QObject{settings}
{}

bool SettingsAdaptor::readSettings(QIODevice &device, QSettings::SettingsMap &map)
{
	const auto fDevice = qobject_cast<QFileDevice*>(&device);
	const auto devName = fDevice ? fDevice->fileName() : QStringLiteral("<unknown>");

	// read info
	QDataStream stream{&device};
	SettingsInfo info;
	stream.startTransaction();
	stream >> info;
	if (!stream.commitTransaction()) {
		qCCritical(logSettings) << "Unable to read settings information from file"
								<< devName;
		return false;
	}

	qCDebug(logSettings) << "Loading settings for table" << info.tableName
						 << "via connection" << info.connectionName
						 << "(seed:" << info.rnd << ")";

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
		loadDataQuery.prepare(QStringLiteral("SELECT Key, Version, Value "
											 "FROM %1 "
											 "ORDER BY Key ASC;")
							  .arg(escName));
		loadDataQuery.exec();
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
		return true;
	} catch (SqlException &e) {
		qCCritical(logSettings) << e.what();
		return false;
	}
}

bool SettingsAdaptor::writeSettings(QIODevice &device, const QSettings::SettingsMap &map)
{

}

QSettings *SettingsAdaptor::settings() const
{
	return static_cast<QSettings*>(parent());
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
