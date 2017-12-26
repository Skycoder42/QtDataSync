#ifndef LOCALSTORE_P_H
#define LOCALSTORE_P_H

#include <functional>
#include <tuple>

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

	Q_PROPERTY(int cacheSize READ cacheSize WRITE setCacheSize RESET resetCacheSize)

public:
	enum ChangeType {
		Exists,
		ExistsDeleted,
		NoExists
	};
	Q_ENUM(ChangeType)

	class SyncScope {
		friend class LocalStore;
		Q_DISABLE_COPY(SyncScope)

	public:
		~SyncScope();

	private:
		ObjectKey key;
		DatabaseRef database;
		QWriteLocker lock;

		SyncScope(const Defaults &defaults, const ObjectKey &key, LocalStore *owner);
	};

	explicit LocalStore(QObject *parent = nullptr);
	explicit LocalStore(const QString &setupName, QObject *parent = nullptr);
	~LocalStore();

	QDir typeDirectory(const ObjectKey &key);
	QJsonObject readJson(const ObjectKey &key, const QString &fileName, int *costs = nullptr);

	// normal store access
	quint64 count(const QByteArray &typeName);
	QStringList keys(const QByteArray &typeName);
	QList<QJsonObject> loadAll(const QByteArray &typeName);

	QJsonObject load(const ObjectKey &key);
	void save(const ObjectKey &key, const QJsonObject &data);
	bool remove(const ObjectKey &key);

	QList<QJsonObject> find(const QByteArray &typeName, const QString &query);
	void clear(const QByteArray &typeName);
	void reset();

	// change access
	bool hasChanges();
	void loadChanges(int limit, const std::function<bool(ObjectKey, quint64, QString)> &visitor);
	void markUnchanged(const ObjectKey &key, quint64 version, bool isDelete);

	// sync access
	SyncScope startSync(const ObjectKey &key);
	std::tuple<QtDataSync::LocalStore::ChangeType, quint64, QByteArray> loadChangeInfo(const SyncScope &scope);
	void storeDeleted(const SyncScope &scope, quint64 version, bool changed, ChangeType localState);
	void commitSync(SyncScope &scope);

	int cacheSize() const;

public Q_SLOTS:
	void setCacheSize(int cacheSize);
	void resetCacheSize();

Q_SIGNALS:
	void dataChanged(const QtDataSync::ObjectKey &key, bool deleted);
	void dataCleared(const QByteArray &typeName);
	void dataResetted();

private Q_SLOTS:
	void onDataChange(QObject *origin, const QtDataSync::ObjectKey &key, const QJsonObject &data, int size);
	void onDataReset(QObject *origin, const QByteArray &typeName);

private:
	Defaults _defaults;
	Logger *_logger;

	DatabaseRef _database;

	QHash<QByteArray, QString> _tableNameCache;
	QCache<ObjectKey, QJsonObject> _dataCache;

	void exec(QSqlQuery &query, const ObjectKey &key = ObjectKey{"any"}) const;
};

}

#endif // LOCALSTORE_P_H
