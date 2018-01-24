#ifndef QTDATASYNC_DATASTOREMODEL_P_H
#define QTDATASYNC_DATASTOREMODEL_P_H

#include "qtdatasync_global.h"
#include "datastoremodel.h"

namespace QtDataSync {

//no export needed
class DataStoreModelPrivate
{
public:
	DataStoreModelPrivate(DataStoreModel *q_ptr);

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
#endif // QTDATASYNC_DATASTOREMODEL_P_H
