#ifndef DATASTORE_P_H
#define DATASTORE_P_H

#include <QtCore/QPointer>

#include "qtdatasync_global.h"
#include "datastore.h"
#include "defaults.h"
#include "logger.h"
#include "localstore_p.h"

namespace QtDataSync {

class Q_DATASYNC_EXPORT DataStorePrivate
{
public:
	DataStorePrivate(DataStore *q, const QString &setupName);

	QByteArray typeName(int metaTypeId) const;

	Defaults defaults;
	Logger *logger;
	QPointer<const QJsonSerializer> serializer;

	LocalStore *store;
};

}

#endif // DATASTORE_P_H
