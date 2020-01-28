#ifndef QTDATASYNC_FIREBASE_REALTIMEDB_SERVERTIMESTAMP_H
#define QTDATASYNC_FIREBASE_REALTIMEDB_SERVERTIMESTAMP_H

#include "qtdatasync_global.h"

#include <QtJsonSerializer/TypeConverter>

namespace QtDataSync::firebase::realtimedb {

struct ServerTimestamp {
	inline bool operator==(const ServerTimestamp &) const { return true; }
	inline bool operator!=(const ServerTimestamp &) const { return false; }
	inline bool operator<(const ServerTimestamp &) const { return false; }
};

class ServerTimestampConverter : public QtJsonSerializer::TypeConverter
{
public:
	QT_JSONSERIALIZER_TYPECONVERTER_NAME(ServerTimestampConverter)
	bool canConvert(int metaTypeId) const override;
	QList<QCborValue::Type> allowedCborTypes(int metaTypeId, QCborTag tag) const override;
	QCborValue serialize(int propertyType, const QVariant &value) const override;
	QVariant deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const override;
};

inline constexpr uint qHash(const ServerTimestamp &, uint seed = 0) {
	return seed;
}

}

Q_DECLARE_METATYPE(QtDataSync::firebase::realtimedb::ServerTimestamp)

#endif // QTDATASYNC_FIREBASE_REALTIMEDB_SERVERTIMESTAMP_H
