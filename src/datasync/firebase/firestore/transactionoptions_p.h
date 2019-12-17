#ifndef QTDATASYNC_FIREBASE_FIRESTORE_TRANSACTIONOPTIONS_H
#define QTDATASYNC_FIREBASE_FIRESTORE_TRANSACTIONOPTIONS_H

#include "qtdatasync_global.h"

#include <QtJsonSerializer/TypeConverter>

namespace QtDataSync::firebase::firestore {

struct TransactionOptions
{
	using ReadOnly = std::optional<QDateTime>;
	using ReadWrite = std::optional<QByteArray>;
	std::variant<ReadOnly, ReadWrite> data;

	bool operator==(const TransactionOptions &other) const;
	bool operator!=(const TransactionOptions &other) const;
};

class TransactionOptionsTypeConverter : public QtJsonSerializer::TypeConverter
{
public:
	QT_JSONSERIALIZER_TYPECONVERTER_NAME(TransactionOptionsTypeConverter)
	bool canConvert(int metaTypeId) const override;
	QList<QCborValue::Type> allowedCborTypes(int metaTypeId, QCborTag tag) const override;
	QCborValue serialize(int propertyType, const QVariant &value) const override;
	QVariant deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::firebase::firestore::TransactionOptions)

#endif // QTDATASYNC_FIREBASE_FIRESTORE_TRANSACTIONOPTIONS_H
