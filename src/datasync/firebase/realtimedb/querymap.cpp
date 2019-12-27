#include "querymap_p.h"
#include <QtCore/QCborMap>
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::realtimedb;

void QueryMap::insert(const QueryMap::Key &key, const QueryMap::Value &value)
{
	// insert ordered
	const auto dPair = std::make_pair(key, value);
	Base::insert(std::upper_bound(begin(), end(), dPair, [](const Element &lhs, const Element &rhs) -> bool {
					 return lhs.second.uploaded() < rhs.second.uploaded();
				 }), dPair);
}



bool QueryMapConverter::canConvert(int metaTypeId) const
{
	return metaTypeId == qMetaTypeId<QueryMap>();
}

QList<QCborValue::Type> QueryMapConverter::allowedCborTypes(int metaTypeId, QCborTag tag) const
{
	Q_UNUSED(metaTypeId)
	Q_UNUSED(tag)
	return {QCborValue::Map};
}

QCborValue QueryMapConverter::serialize(int propertyType, const QVariant &value) const
{
	Q_UNUSED(propertyType)
	QCborMap map;
	for (const auto &element : value.value<QueryMap>()) {
		map.insert(element.first,
				   helper()->serializeSubtype(qMetaTypeId<Data>(),
											  QVariant::fromValue(element.second),
											  element.first.toUtf8()));
	}
	return map;
}

QVariant QueryMapConverter::deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const
{
	Q_UNUSED(propertyType)
	const auto cborMap = value.toMap();
	QueryMap map;
	map.reserve(cborMap.size());
	for (const auto &element : cborMap) {
		map.insert(element.first.toString(),
				   helper()->deserializeSubtype(qMetaTypeId<Data>(),
												element.second,
												parent,
												element.first.toString().toUtf8())
					   .value<Data>());
	}
	return QVariant::fromValue(map);
}
