#include "structuredquery_p.h"
#include <QtCore/QCborMap>
#include <QtCore/QCborArray>
#include <QtJsonSerializer/exception.h>
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::firestore;

bool StructuredQuery::operator==(const StructuredQuery &other) const
{
	return tables == other.tables &&
		   timeStamp == other.timeStamp &&
		   limit == other.limit &&
		   offset == other.offset;
}

bool StructuredQuery::operator!=(const StructuredQuery &other) const
{
	return tables != other.tables ||
		   timeStamp != other.timeStamp ||
		   limit != other.limit ||
		   offset != other.offset;
}



bool StructuredQueryTypeConverter::canConvert(int metaTypeId) const
{
	return metaTypeId == qMetaTypeId<StructuredQuery>();
}

QList<QCborValue::Type> StructuredQueryTypeConverter::allowedCborTypes(int metaTypeId, QCborTag tag) const
{
	Q_UNUSED(metaTypeId)
	Q_UNUSED(tag)
	return {QCborValue::Map};
}

QCborValue StructuredQueryTypeConverter::serialize(int propertyType, const QVariant &value) const
{
	Q_UNUSED(propertyType)
	const auto query = value.value<StructuredQuery>();

	QCborArray fromValues;
	for (const auto &table : query.tables) {
		fromValues.append(QCborMap {
			{QStringLiteral("collectionId"), table}
		});
	}

	return QCborMap {
		{QStringLiteral("from"), fromValues},
		{QStringLiteral("where"), QCborMap{
			{QStringLiteral("fieldFilter"), QCborMap{
				{QStringLiteral("field"), QCborMap{
					{QStringLiteral("collectionId"), QStringLiteral("timestamp")}
				}},
				{QStringLiteral("op"), QStringLiteral("GREATER_THAN_OR_EQUAL")},
				{QStringLiteral("value"), QCborMap{
					{QStringLiteral("timestampValue"), query.timeStamp.toString(Qt::ISODateWithMs)}
				}},
			}}
		}},
		{QStringLiteral("orderBy"), QCborArray{
			QCborMap {
				{QStringLiteral("field"), QCborMap{
					{QStringLiteral("fieldPath"), QStringLiteral("timestamp")}
				}},
				{QStringLiteral("direction"), QStringLiteral("ASCENDING")}
			}
		}},
		{QStringLiteral("limit"), query.limit},
		{QStringLiteral("offset"), query.offset}
	};
}

QVariant StructuredQueryTypeConverter::deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const
{
	// queries are only sent, never received
	Q_UNUSED(propertyType)
	Q_UNUSED(value)
	Q_UNUSED(parent)
	throw QtJsonSerializer::DeserializationException{"StructuredQueries cannot be deserialized"};
}
