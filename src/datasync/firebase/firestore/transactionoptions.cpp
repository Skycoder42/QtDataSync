#include "transactionoptions_p.h"
#include <QtCore/QCborMap>
#include <QtJsonSerializer/exception.h>
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::firestore;

bool TransactionOptions::operator==(const TransactionOptions &other) const
{
	return data == other.data;
}

bool TransactionOptions::operator!=(const TransactionOptions &other) const
{
	return data != other.data;
}



bool TransactionOptionsTypeConverter::canConvert(int metaTypeId) const
{
	return metaTypeId == qMetaTypeId<TransactionOptions>();
}

QList<QCborValue::Type> TransactionOptionsTypeConverter::allowedCborTypes(int metaTypeId, QCborTag tag) const
{
	Q_UNUSED(metaTypeId)
	Q_UNUSED(tag)
	return {QCborValue::Map};
}

QCborValue TransactionOptionsTypeConverter::serialize(int propertyType, const QVariant &value) const
{
	Q_UNUSED(propertyType)
	const auto options = value.value<TransactionOptions>();
	if (std::holds_alternative<TransactionOptions::ReadOnly>(options.data)) {
		QCborMap rMap;
		if (const auto rTime = std::get<TransactionOptions::ReadOnly>(options.data); rTime)
			rMap.insert(QStringLiteral("readTime"), rTime->toString(Qt::ISODateWithMs));
		return QCborMap {
			{QStringLiteral("readOnly"), rMap}
		};
	} else if (std::holds_alternative<TransactionOptions::ReadWrite>(options.data)) {
		QCborMap rwMap;
		if (const auto retry = std::get<TransactionOptions::ReadWrite>(options.data); retry)
			rwMap.insert(QStringLiteral("retryTransaction"), *retry);
		return QCborMap {
			{QStringLiteral("readWrite"), rwMap}
		};
	}
	Q_UNREACHABLE();
	return {};
}

QVariant TransactionOptionsTypeConverter::deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const
{
	// queries are only sent, never received
	Q_UNUSED(propertyType)
	Q_UNUSED(value)
	Q_UNUSED(parent)
	throw QtJsonSerializer::DeserializationException{"TransactionOptions cannot be deserialized"};
}
