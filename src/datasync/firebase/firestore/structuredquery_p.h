#ifndef QTDATASYNC_FIREBASE_FIRESTORE_QUERY_H
#define QTDATASYNC_FIREBASE_FIRESTORE_QUERY_H

#include "qtdatasync_global.h"

#include <QtCore/QObject>

#include <QtJsonSerializer/TypeConverter>

namespace QtDataSync::firebase::firestore {

struct StructuredQuery
{
	QStringList tables;
	QDateTime timeStamp;
	int limit = 100;
	int offset = 0;
};

class StructuredQueryTypeConverter : public QtJsonSerializer::TypeConverter
{
public:
	QT_JSONSERIALIZER_TYPECONVERTER_NAME(StructuredQueryTypeConverter)
	bool canConvert(int metaTypeId) const override;
	QList<QCborValue::Type> allowedCborTypes(int metaTypeId, QCborTag tag) const override;
	QCborValue serialize(int propertyType, const QVariant &value) const override;
	QVariant deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::firebase::firestore::StructuredQuery)

#endif // QTDATASYNC_FIREBASE_FIRESTORE_QUERY_H
