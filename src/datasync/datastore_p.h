#ifndef QTDATASYNC_DATASTORE_P_H
#define QTDATASYNC_DATASTORE_P_H

#include <QtCore/QPointer>

#include "qtdatasync_global.h"
#include "datastore.h"
#include "defaults.h"
#include "logger.h"
#include "localstore_p.h"

namespace QtDataSync {

//no export needed
class DataStorePrivate
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

#endif // QTDATASYNC_DATASTORE_P_H
