#ifndef LOCALSTORE_P_H
#define LOCALSTORE_P_H

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QCache>
#include <QtCore/QReadWriteLock>
#include <QtCore/QJsonObject>

#include <QtSql/QSqlDatabase>

#include "qtdatasync_global.h"
#include "objectkey.h"
#include "defaults.h"
#include "logger.h"
#include "exception.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT LocalStoreEmitter : public QObject
{
	Q_OBJECT

public:
	LocalStoreEmitter(QObject *parent = nullptr);

Q_SIGNALS:
	void dataChanged(QObject *origin, const QtDataSync::ObjectKey &key, const QJsonObject data, int size);
	void dataResetted(QObject *origin, const QByteArray &typeName = {});
};

class Q_DATASYNC_EXPORT LocalStore : public QObject //TODO use const where useful
{
	Q_OBJECT

public:
	explicit LocalStore(QObject *parent = nullptr);
	explicit LocalStore(const QString &setupName, QObject *parent = nullptr);

	quint64 count(const QByteArray &typeName);
	QStringList keys(const QByteArray &typeName);
	QList<QJsonObject> loadAll(const QByteArray &typeName);

	QJsonObject load(const ObjectKey &key);
	void save(const ObjectKey &key, const QJsonObject &data);
	bool remove(const ObjectKey &key);

	QList<QJsonObject> find(const QByteArray &typeName, const QString &query);
	void clear(const QByteArray &typeName);
	void reset();

Q_SIGNALS:
	void dataChanged(const QtDataSync::ObjectKey &key, bool deleted);
	void dataCleared(const QByteArray &typeName);
	void dataResetted();

private Q_SLOTS:
	void onDataChange(QObject *origin, const QtDataSync::ObjectKey &key, const QJsonObject &data, int size);
	void onDataReset(QObject *origin, const QByteArray &typeName);

private:
	static QReadWriteLock globalLock;

	Defaults defaults;
	Logger *logger;

	DatabaseRef database;

	QHash<QByteArray, QString> tableNameCache;
	QCache<ObjectKey, QJsonObject> dataCache;

	QString getTable(const QByteArray &typeName, bool allowCreate = false);
	QDir typeDirectory(const QString &tableName, const ObjectKey &key);
	QJsonObject readJson(const QString &tableName, const QString &fileName, const ObjectKey &key, int *costs = nullptr);
};

}

#endif // LOCALSTORE_P_H
