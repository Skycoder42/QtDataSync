#ifndef QTDATASYNC_DATABASEPROXY_H
#define QTDATASYNC_DATABASEPROXY_H

#include "qtdatasync_global.h"
#include "engine.h"
#include "databasewatcher_p.h"
#include "cloudtransformer.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QHash>
#include <QtCore/QQueue>
#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>

namespace QtDataSync {

class Q_DATASYNC_EXPORT DatabaseProxy : public QObject
{
	Q_OBJECT

public:
	enum class Type {
		Local,
		Cloud,
		Both
	};
	Q_ENUM(Type)

	enum class TableStateFlag {
		Clean = 0x00,
		LocalDirty = 0x01,
		CloudDirty = 0x02,
		Blocked = 0x04,

		AllDirty = (LocalDirty | CloudDirty)
	};
	Q_DECLARE_FLAGS(TableState, TableStateFlag)
	Q_ENUM(TableState)

	template <typename TFn>
	struct FnInfo{};
	template <typename TRet, typename... TArgs>
	struct FnInfo<TRet (DatabaseWatcher::*)(TArgs...)> {
		using Ret = TRet;
	};

	template <typename TFn>
	using FnReturn = std::enable_if_t<!std::is_void_v<typename FnInfo<TFn>::Ret>, typename FnInfo<TFn>::Ret>;
	template <typename TFn>
	using VoidReturn = std::enable_if_t<std::is_void_v<typename FnInfo<TFn>::Ret>, void>;

	using DirtyTableInfo = std::optional<std::pair<QString, QDateTime>>;

	explicit DatabaseProxy(Engine *engine = nullptr);

	// watcher access
	DatabaseWatcher *watcher(QSqlDatabase &&database);  // gets or creates a watcher for the given database connection
	void dropWatcher(QSqlDatabase &&database);  // drops a watcher without removing synced tables

	// table management
	DirtyTableInfo nextDirtyTable(Type type) const;
	void fillDirtyTables(Type type);
	void markTableDirty(const QString &name, Type type);
	void clearDirtyTable(const QString &name, Type type);
	void resetAll();

	// watcher proxy
	template <typename TFn, typename TFirst, typename... TArgs>
	FnReturn<TFn> call(TFn fn, TFirst &&key, TArgs&&... args);
	template <typename TFn, typename TFirst, typename... TArgs>
	VoidReturn<TFn> call(TFn fn, TFirst &&key, TArgs&&... args);
	template <typename TRet, typename TFirst, typename... TArgs>
	std::function<TRet(TFirst, TArgs...)> bind(TRet (DatabaseWatcher::*fn)(TFirst, TArgs...));

Q_SIGNALS:
	void triggerSync(bool uploadOnly, QPrivateSignal = {});
	void databaseError(Engine::ErrorType type,
					   const QString &message,
					   const QVariant &key,
					   QPrivateSignal = {});

private Q_SLOTS:
	void tableAdded(const QString &name);
	void tableRemoved(const QString &name);
	void watcherError(DatabaseWatcher::ErrorScope scope,
					  const QString &message,
					  const QVariant &key,
					  const QSqlError &sqlError);

private:
	template <typename TKey>
	struct KeyInfo;

	struct TableInfo {
		QPointer<DatabaseWatcher> watcher;
		TableState state = TableStateFlag::Clean;

		inline TableInfo(DatabaseWatcher *watcher = nullptr) :
			watcher{watcher}
		{}
	};
	// TODO watcher ref count to remove on last table removed

	QHash<QString, DatabaseWatcher*> _watchers;  // dbName -> watcher
	QHash<QString, TableInfo> _tables;  // tableName -> state

	static TableStateFlag toState(Type type);
};

Q_DECLARE_LOGGING_CATEGORY(logDbProxy)

template <>
struct DatabaseProxy::KeyInfo<QString> {
	static inline QString key(const QString &tableName) {
		return tableName;
	}
};

template <>
struct DatabaseProxy::KeyInfo<ObjectKey> {
	static inline QString key(const ObjectKey &key) {
		return key.typeName;
	}
};

template <>
struct DatabaseProxy::KeyInfo<LocalData> {
	static inline QString key(const LocalData &data) {
		return data.key().typeName;
	}
};

template<typename TFn, typename TFirst, typename... TArgs>
DatabaseProxy::FnReturn<TFn> DatabaseProxy::call(TFn fn, TFirst &&key, TArgs&&... args)
{
	const auto tableName = KeyInfo<std::decay_t<TFirst>>::key(key);
	auto tInfo = _tables[tableName];
	if (!tInfo.watcher) {
		qCWarning(logDbProxy) << "Unknown table" << tableName;
		return {};
	}
	return (tInfo.watcher->*fn)(std::forward<TFirst>(key), std::forward<TArgs>(args)...);
}

template<typename TFn, typename TFirst, typename... TArgs>
DatabaseProxy::VoidReturn<TFn> DatabaseProxy::call(TFn fn, TFirst &&key, TArgs&&... args)
{
	const auto tableName = KeyInfo<std::decay_t<TFirst>>::key(key);
	auto tInfo = _tables[tableName];
	if (!tInfo.watcher) {
		qCWarning(logDbProxy) << "Unknown table" << tableName;
		return;
	}
	return (tInfo.watcher->*fn)(std::forward<TFirst>(key), std::forward<TArgs>(args)...);
}

template<typename TRet, typename TFirst, typename... TArgs>
std::function<TRet(TFirst, TArgs...)> DatabaseProxy::bind(TRet (DatabaseWatcher::*fn)(TFirst, TArgs...))
{
	return [this, fn](TFirst &&key, TArgs&&... args) -> TRet {
		return call(fn, std::forward<TFirst>(key), std::forward<TArgs>(args)...);
	};
}

}

Q_DECLARE_OPERATORS_FOR_FLAGS(QtDataSync::DatabaseProxy::TableState)

#endif // QTDATASYNC_DATABASEPROXY_H
