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

class Q_DATASYNC_EXPORT LocalStore : public QObject
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

	class Q_DATASYNC_EXPORT SyncScope {
		friend class LocalStore;
		Q_DISABLE_COPY(SyncScope)

	public:
		SyncScope(SyncScope &&other);
		~SyncScope();

	private:
		struct Q_DATASYNC_EXPORT Private {
			ObjectKey key;
			DatabaseRef database;
			QWriteLocker lock;
			std::function<void()> afterCommit;

			Private(const Defaults &defaults, const ObjectKey &key, LocalStore *owner);
		};
		QScopedPointer<Private> d;

		SyncScope(const Defaults &defaults, const ObjectKey &key, LocalStore *owner);
	};

	explicit LocalStore(const Defaults &defaults, QObject *parent = nullptr);
	~LocalStore();

	QJsonObject readJson(const ObjectKey &key, const QString &filePath, int *costs = nullptr) const;

	// normal store access
	quint64 count(const QByteArray &typeName) const;
	QStringList keys(const QByteArray &typeName) const;
	QList<QJsonObject> loadAll(const QByteArray &typeName) const;

	QJsonObject load(const ObjectKey &key) const;
	void save(const ObjectKey &key, const QJsonObject &data);
	bool remove(const ObjectKey &key);

	QList<QJsonObject> find(const QByteArray &typeName, const QString &query) const;
	void clear(const QByteArray &typeName);
	void reset();

	// change access
	void loadChanges(int limit, const std::function<bool(ObjectKey, quint64, QString)> &visitor) const;
	void markUnchanged(const ObjectKey &key, quint64 version, bool isDelete);

	// sync access
	SyncScope startSync(const ObjectKey &key) const;
	std::tuple<QtDataSync::LocalStore::ChangeType, quint64, QString, QByteArray> loadChangeInfo(SyncScope &scope) const;
	void updateVersion(SyncScope &scope,
					   quint64 oldVersion,
					   quint64 newVersion,
					   bool changed);
	void storeChanged(SyncScope &scope,
					  quint64 version,
					  const QString &filePath,
					  const QJsonObject &data,
					  bool changed,
					  ChangeType localState);
	void storeDeleted(SyncScope &scope,
					  quint64 version,
					  bool changed,
					  ChangeType localState);
	void markUnchanged(SyncScope &scope,
					   quint64 oldVersion,
					   bool isDelete);
	void commitSync(SyncScope &scope) const;

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
	mutable QCache<ObjectKey, QJsonObject> _dataCache;

	QDir typeDirectory(const ObjectKey &key) const;
	QString filePath(const QDir &typeDir, const QString &baseName) const;
	QString filePath(const ObjectKey &key, const QString &baseName) const;

	void exec(QSqlQuery &query, const ObjectKey &key = ObjectKey{"any"}) const;

	Q_REQUIRED_RESULT std::function<void()> storeChangedImpl(const DatabaseRef &db,
															 const ObjectKey &key,
															 quint64 version,
															 const QString &filePath,
															 const QJsonObject &data,
															 bool changed,
															 bool existing,
															 const QWriteLocker &lock);
	void markUnchangedImpl(const DatabaseRef &db,
						   const ObjectKey &key,
						   quint64 version,
						   bool isDelete,
						   const QWriteLocker &);
};

}

#endif // LOCALSTORE_P_H
