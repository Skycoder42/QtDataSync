#ifndef DATASTOREMODEL_P_H
#define DATASTOREMODEL_P_H

#include "qtdatasync_global.h"
#include "datastoremodel.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT DataStoreModelPrivate
{
public:
	DataStoreModelPrivate(DataStoreModel *q_ptr, DataStore *store);

	DataStoreModel *q;
	DataStore *store;
	bool editable;

	int type;
	bool isObject;
	QHash<int, QByteArray> roleNames;

	QStringList keyList;
	QVariantHash dataHash;

	QStringList activeKeys();

	void createRoleNames();
	void clearHashObjects();
	void deleteObject(const QVariant &value);
	bool testValid(const QModelIndex &index, int role) const;

	QVariant readProperty(const QString &key, const QByteArray &property);
	bool writeProperty(const QString &key, const QByteArray &property, const QVariant &value);
};

}
#endif // DATASTOREMODEL_P_H
