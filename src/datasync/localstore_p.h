#ifndef QTDATASYNC_LOCALSTORE_P_H
#define QTDATASYNC_LOCALSTORE_P_H

#include <functional>
#include <tuple>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QJsonObject>
#include <QtCore/QUuid>

#include <QtSql/QSqlDatabase>

#include "qtdatasync_global.h"
#include "objectkey.h"
#include "defaults.h"
#include "logger.h"
#include "exception.h"
#include "datastore.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT LocalStore : public QObject
{
	Q_OBJECT

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
		//no export needed
		struct Private {
			ObjectKey key;
			DatabaseRef database;
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

	QList<QJsonObject> find(const QByteArray &typeName, const QString &query, DataStore::SearchMode mode) const;
	void clear(const QByteArray &typeName);
	void reset(bool keepData);

	// change access
	quint32 changeCount() const;
	void loadChanges(int limit, const std::function<bool(ObjectKey, quint64, QString, QUuid)> &visitor) const; //(key, version, file, device)
	void markUnchanged(const ObjectKey &key, quint64 version, bool isDelete);
	void removeDeviceChange(const ObjectKey &key, const QUuid &deviceId);

	// sync access
	SyncScope startSync(const ObjectKey &key) const;
	std::tuple<QtDataSync::LocalStore::ChangeType, quint64, QString, QByteArray> loadChangeInfo(SyncScope &scope) const; //(changetype, version, filename, checksum)
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

	void prepareAccountAdded(const QUuid &deviceId);

Q_SIGNALS:
	void dataChanged(const QtDataSync::ObjectKey &key, bool deleted);
	void dataCleared(const QByteArray &typeName);
	void dataResetted();

private:
	Defaults _defaults;
	Logger *_logger;
	EmitterAdapter *_emitter;
	DatabaseRef _database;

	QDir typeDirectory(const ObjectKey &key) const;
	QString filePath(const QDir &typeDir, const QString &baseName) const;
	QString filePath(const ObjectKey &key, const QString &baseName) const;

	void beginReadTransaction(const ObjectKey &key = ObjectKey{"any"}) const;
	void beginWriteTransaction(const ObjectKey &key = ObjectKey{"any"}, bool exclusive = false);
	void exec(QSqlQuery &query, const ObjectKey &key = ObjectKey{"any"}) const;

	Q_REQUIRED_RESULT std::function<void ()> storeChangedImpl(const DatabaseRef &db,
																 const ObjectKey &key,
																 quint64 version,
																 const QString &filePath,
																 const QJsonObject &data,
																 bool changed,
																 bool existing);
	void markUnchangedImpl(const DatabaseRef &db,
						   const ObjectKey &key,
						   quint64 version,
						   bool isDelete);
};

}

#endif // QTDATASYNC_LOCALSTORE_P_H
