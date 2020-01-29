#include "datasetid.h"
#include <QtCore/QDebug>
using namespace QtDataSync;

bool DatasetId::isValid() const
{
	return !tableName.isEmpty();
}

QString DatasetId::toString() const
{
	return QStringLiteral("[%1:%2]").arg(tableName, key);
}

bool DatasetId::operator==(const QtDataSync::DatasetId &other) const
{
	return tableName == other.tableName &&
		   key == other.key;
}

bool DatasetId::operator!=(const QtDataSync::DatasetId &other) const
{
	return tableName != other.tableName ||
					   key != other.key;
}

uint QtDataSync::qHash(const QtDataSync::DatasetId &key, uint seed)
{
	return qHash(key.tableName, seed) ^ qHash(key.key, ~seed);
}

QDebug QtDataSync::operator<<(QDebug debug, const DatasetId &key)
{
	QDebugStateSaver saver{debug};
	debug.nospace().noquote() << '[' << key.tableName << ':' << key.key << ']';
	return debug;
}
