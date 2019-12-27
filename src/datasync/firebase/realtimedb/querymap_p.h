#ifndef QTDATASYNC_FIREBASE_REALTIMEDB_QUERYMAP_H
#define QTDATASYNC_FIREBASE_REALTIMEDB_QUERYMAP_H

#include "data.h"

#include <QtJsonSerializer/typeconverter.h>

namespace QtDataSync::firebase::realtimedb {

class QueryMap : public QList<std::pair<QString, Data>>
{
public:
	using Base = QList<std::pair<QString, Data>>;
	using Key = QString;
	using Value = Data;
	using Element = std::pair<const Key, Value>;

	void insert(const Key &key, const Value &value);
};

class QueryMapConverter : public QtJsonSerializer::TypeConverter
{
public:
	QT_JSONSERIALIZER_TYPECONVERTER_NAME(QueryMapConverter)
	bool canConvert(int metaTypeId) const override;
	QList<QCborValue::Type> allowedCborTypes(int metaTypeId, QCborTag tag) const override;
	QCborValue serialize(int propertyType, const QVariant &value) const override;
	QVariant deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const override;
};

}

Q_DECLARE_METATYPE(QtDataSync::firebase::realtimedb::QueryMap)

#endif // QTDATASYNC_FIREBASE_REALTIMEDB_QUERYMAP_H
