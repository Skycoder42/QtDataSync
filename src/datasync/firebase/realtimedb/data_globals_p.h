#ifndef QTDATASYNC_FIREBASE_REALTIMEDB_DATA_GLOBALS_P_H
#define QTDATASYNC_FIREBASE_REALTIMEDB_DATA_GLOBALS_P_H

#include "qtdatasync_global.h"
#include "servertimestamp_p.h"

#include <variant>
#include <optional>

#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QVersionNumber>

namespace QtDataSync::firebase::realtimedb {

using Timestamp = std::variant<QDateTime, ServerTimestamp>;
using Content = std::variant<std::optional<QJsonObject>, bool>;
using Version = std::optional<QVersionNumber>;

}

// WORKAROUND: declare qHash for std::variant and std::optional
namespace std {

template <typename... TArgs>
inline constexpr uint qHash(const variant<TArgs...> &var, uint seed = 0) {
	return std::visit([seed](const auto &data) {
		return ::qHash(data, seed);
	}, var) ^ static_cast<uint>(var.index());
}

template <typename T>
inline constexpr uint qHash(const optional<T> &opt, uint seed = 0) {
	return opt ? ::qHash(*opt, seed) : ~seed;
}

}

Q_DECLARE_METATYPE(std::optional<QJsonObject>)
Q_DECLARE_METATYPE(QtDataSync::firebase::realtimedb::Timestamp)
Q_DECLARE_METATYPE(QtDataSync::firebase::realtimedb::Content)
Q_DECLARE_METATYPE(QtDataSync::firebase::realtimedb::Version)

#endif // DATA_GLOBALS_P_H
