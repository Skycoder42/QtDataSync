#include "servertimestamp_p.h"
#include <QtCore/QCborMap>
#include <QtJsonSerializer/DeserializationException>
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::realtimedb;

bool ServerTimestampConverter::canConvert(int metaTypeId) const
{
	return metaTypeId == qMetaTypeId<ServerTimestamp>();
}

QList<QCborValue::Type> ServerTimestampConverter::allowedCborTypes(int metaTypeId, QCborTag tag) const
{
	Q_UNUSED(metaTypeId)
	Q_UNUSED(tag)
	return {};
}

QCborValue ServerTimestampConverter::serialize(int propertyType, const QVariant &value) const
{
	Q_UNUSED(propertyType)
	Q_UNUSED(value)
	return QCborMap {
		{QStringLiteral(".sv"), QStringLiteral("timestamp")}
	};
}

QVariant ServerTimestampConverter::deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const
{
	Q_UNUSED(propertyType)
	Q_UNUSED(value)
	Q_UNUSED(parent)
	throw QtJsonSerializer::DeserializationException{"ServerTimestamps cannot be deserialized"};
}
