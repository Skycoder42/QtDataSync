#ifndef QTDATASYNC_DATASTOREMODEL_P_H
#define QTDATASYNC_DATASTOREMODEL_P_H

#include "qtdatasync_global.h"
#include "datastoremodel.h"

#include <functional>

namespace QtDataSync {

class Q_DATASYNC_EXPORT DataStoreModelPrivate
{
public:
	DataStoreModelPrivate(DataStoreModel *q_ptr, AsyncDataStore *store);

	DataStoreModel *q;
	AsyncDataStore *store;

	int type;
	bool isObject;
	QHash<int, QByteArray> roleNames;

	QStringList keyList;
	QHash<QString, QVariant> dataHash;


	template <typename TRet, typename... TArgs>
	Q_DECL_CONSTEXPR std::function<TRet(TArgs...)> fn(TRet(DataStoreModelPrivate::* member)(TArgs...)) {
		return [this, member](TArgs... args) -> TRet {
			return (this->*member)(args...);
		};
	}

	void createRoleNames();
	void clearHashObjects();
	void deleteObject(const QVariant &value);
	bool testValid(const QModelIndex &index, int role) const;

	void onLoadAll(QVariant data);
	void onLoad(QVariant data);
	void onError(const QException &exception);

	QString readKey(QVariant data);
	QVariant readProperty(const QString &key, const QByteArray &property);
	bool writeProperty(const QString &key, const QByteArray &property, const QVariant &value);
};

}

#endif // QTDATASYNC_DATASTOREMODEL_P_H
