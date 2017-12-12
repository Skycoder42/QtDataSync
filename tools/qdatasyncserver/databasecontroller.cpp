#include "databasecontroller.h"
#include "app.h"

#include <QtCore/QJsonDocument>

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <QtConcurrent/QtConcurrentRun>

DatabaseController::DatabaseController(QObject *parent) :
	QObject(parent),
	multiThreaded(false),
	threadStore()
{
	QtConcurrent::run(qApp->threadPool(), this, &DatabaseController::initDatabase);
}

void DatabaseController::cleanupDevices(quint64 offlineSinceDays)
{
	QtConcurrent::run(qApp->threadPool(), [this, offlineSinceDays]() {
		auto db = threadStore.localData().database();
		if(!db.transaction()) {
			emit cleanupOperationDone(-1,
									  QStringLiteral("Failed to create transaction with error: %1")
									  .arg(db.lastError().text()));
			return;
		}

		//TODO IMPLEMENT
		Q_UNIMPLEMENTED();

		if(db.commit())
			emit cleanupOperationDone(0);
		else {
			emit cleanupOperationDone(-1,
									  QStringLiteral("Failed to commit transaction with error: %1")
									  .arg(db.lastError().text()));
		}
	});
}

void DatabaseController::initDatabase()
{
	auto db = threadStore.localData().database();
	if(!db.isOpen())
		emit databaseInitDone(false);

//#define AUTO_DROP_TABLES
#ifdef AUTO_DROP_TABLES
	QSqlQuery dropQuery(db);
	if(!dropQuery.exec(QStringLiteral("DROP TABLE IF EXISTS devices, users CASCADE"))) {
		qWarning() << "Failed to drop tables with error:"
				   << qPrintable(dropQuery.lastError().text());
	} else
		qInfo() << "Dropped all existing tables";
#endif

	if(!db.tables().contains(QStringLiteral("users"))) {
		QSqlQuery createUsers(db);
		if(!createUsers.exec(QStringLiteral("CREATE TABLE users ( "
										   "	id			BIGSERIAL PRIMARY KEY NOT NULL, "
										   "	keycount	INT NOT NULL DEFAULT 0 "
										   ")"))) {
			qCritical() << "Failed to create users table with error:"
						<< qPrintable(createUsers.lastError().text());
			emit databaseInitDone(false);
			return;
		}
	}


	if(!db.tables().contains(QStringLiteral("devices"))) {
		QSqlQuery createDevices(db);
		if(!createDevices.exec(QStringLiteral("CREATE TABLE devices ( "
											  "		id			UUID PRIMARY KEY NOT NULL, "
											  "		userid		BIGINT NOT NULL REFERENCES users(id), "
											  "		name		TEXT NOT NULL, "
											  "		signscheme	TEXT NOT NULL, "
											  "		signkey		BYTEA NOT NULL, "
											  "		cryptscheme	TEXT NOT NULL, "
											  "		cryptkey	BYTEA NOT NULL, "
											  "		lastlogin	DATE NOT NULL DEFAULT 'today' "
											  ")"))) {
			qCritical() << "Failed to create devices table with error:"
						<< qPrintable(createDevices.lastError().text());
			emit databaseInitDone(false);
			return;
		}
	}

	//TODO IMPLEMENT

	emit databaseInitDone(true);
}

QString DatabaseController::jsonToString(const QJsonObject &object) const
{
	return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QJsonObject DatabaseController::stringToJson(const QString &data) const
{
	return QJsonDocument::fromJson(data.toUtf8()).object();
}



DatabaseController::DatabaseWrapper::DatabaseWrapper() :
	dbName(QUuid::createUuid().toString())
{
	auto config = qApp->configuration();
	auto db = QSqlDatabase::addDatabase(config->value(QStringLiteral("database/driver"), QStringLiteral("QPSQL")).toString(), dbName);
	db.setDatabaseName(config->value(QStringLiteral("database/name"), QCoreApplication::applicationName()).toString());
	db.setHostName(config->value(QStringLiteral("database/host"), QStringLiteral("localhost")).toString());
	db.setPort(config->value(QStringLiteral("database/port"), 5432).toInt());
	db.setUserName(config->value(QStringLiteral("database/username")).toString());
	db.setPassword(config->value(QStringLiteral("database/password")).toString());
	db.setConnectOptions(config->value(QStringLiteral("database/options")).toString());

	if(!db.open()) {
		qCritical() << "Failed to open database with error:"
					<< qPrintable(db.lastError().text());
	} else
		qDebug() << "DB connected for thread" << QThread::currentThreadId();
}

DatabaseController::DatabaseWrapper::~DatabaseWrapper()
{
	QSqlDatabase::database(dbName).close();
	QSqlDatabase::removeDatabase(dbName);
	qDebug() << "DB disconnected for thread" << QThread::currentThreadId();
}

QSqlDatabase DatabaseController::DatabaseWrapper::database() const
{
	return QSqlDatabase::database(dbName);
}
