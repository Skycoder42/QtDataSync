#ifndef QTDATASYNC_FIREBASE_REALTIMEDB_SERVERTIMESTAMP_H
#define QTDATASYNC_FIREBASE_REALTIMEDB_SERVERTIMESTAMP_H

#include "qtdatasync_global.h"

#include <variant>
#include <optional>

#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>

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

using Timestamp = std::variant<QDateTime, ServerTimestamp>;
using Content = std::variant<std::optional<QJsonObject>, bool>;

}

// WORKAROUND: declare qHash std::variant and std::optional
namespace std {

template <typename... TArgs>
inline constexpr uint qHash(const variant<TArgs...> &var, uint seed = 0) {
	return std::visit([seed](const auto &data) {
		return ::qHash(data, seed);
	}, var);
}

template <typename T>
inline constexpr uint qHash(const optional<T> &opt, uint seed = 0) {
	return opt ? ::qHash(*opt, seed) : ~seed;
}

}

Q_DECLARE_METATYPE(std::optional<QJsonObject>)
Q_DECLARE_METATYPE(QtDataSync::firebase::realtimedb::ServerTimestamp)
Q_DECLARE_METATYPE(QtDataSync::firebase::realtimedb::Timestamp)
Q_DECLARE_METATYPE(QtDataSync::firebase::realtimedb::Content)

#endif // QTDATASYNC_FIREBASE_REALTIMEDB_SERVERTIMESTAMP_H
