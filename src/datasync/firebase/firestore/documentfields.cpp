#include "documentfields_p.h"
#include <QtCore/QCborMap>
#include <QtCore/QCborArray>
#include <QtJsonSerializer/exception.h>
using namespace QtDataSync;
using namespace QtDataSync::firebase;
using namespace QtDataSync::firebase::firestore;

DocumentFields::DocumentFields(std::initializer_list<std::pair<QString, QVariant> > args) :
	QVariantMap{std::move(args)}
{}

DocumentFields::DocumentFields(const QVariantMap &other) :
	QVariantMap{other}
{}

DocumentFields::DocumentFields(QVariantMap &&other) noexcept :
	QVariantMap{std::move(other)}
{}

DocumentFields &DocumentFields::operator=(const QVariantMap &other)
{
	QVariantMap::operator=(other);
	return *this;
}

DocumentFields &DocumentFields::operator=(QVariantMap &&other) noexcept
{
	QVariantMap::operator=(std::move(other));
	return *this;
}

bool DocumentFields::operator==(const DocumentFields &other) const
{
	return QVariantMap::operator==(other);
}

bool DocumentFields::operator!=(const DocumentFields &other) const
{
	return QVariantMap::operator!=(other);
}



bool DocumentFieldsTypeConverter::canConvert(int metaTypeId) const
{
	return metaTypeId == qMetaTypeId<DocumentFields>();
}

QList<QCborValue::Type> DocumentFieldsTypeConverter::allowedCborTypes(int metaTypeId, QCborTag tag) const
{
	Q_UNUSED(metaTypeId)
	Q_UNUSED(tag)
	return {QCborValue::Map};
}

QCborValue DocumentFieldsTypeConverter::serialize(int propertyType, const QVariant &value) const
{
	Q_UNUSED(propertyType)
	const auto fields = value.value<DocumentFields>();
	QCborMap map;
	for (auto it = fields.begin(), end = fields.end(); it != end; ++it)
		map.insert(it.key(), serializeValue(it.value()));
	return map;
}

QVariant DocumentFieldsTypeConverter::deserializeCbor(int propertyType, const QCborValue &value, QObject *parent) const
{
	Q_UNUSED(propertyType)
	Q_UNUSED(parent)
	DocumentFields fields;
	for (const auto &field : value.toMap())
		fields.insert(field.first.toString(), deserializeValue(field.second));
	return fields;
}

QCborValue DocumentFieldsTypeConverter::serializeValue(const QVariant &value) const
{
	switch (value.userType()) {
	case QMetaType::UnknownType:
	case QMetaType::Nullptr:
		return QCborMap {{QStringLiteral("nullValue"), QCborSimpleType::Null}};
	case QMetaType::Bool:
		return QCborMap {{QStringLiteral("booleanValue"), value.toBool()}};
	case QMetaType::Char:
	case QMetaType::SChar:
	case QMetaType::Short:
	case QMetaType::Int:
	case QMetaType::Long:
	case QMetaType::LongLong:
		return QCborMap {{QStringLiteral("integerValue"), QString::number(value.toLongLong())}};
	case QMetaType::UChar:
	case QMetaType::UShort:
	case QMetaType::UInt:
	case QMetaType::ULong:
	case QMetaType::ULongLong:
		return QCborMap {{QStringLiteral("integerValue"), QString::number(value.toULongLong())}};
	case QMetaType::Double:
	case QMetaType::Float:
		return QCborMap {{QStringLiteral("doubleValue"), value.toDouble()}};
	case QMetaType::QDateTime:
		return QCborMap {{QStringLiteral("timestampValue"), value.toDateTime().toUTC().toString(Qt::ISODateWithMs)}};
	case QMetaType::QString:
		return QCborMap {{QStringLiteral("stringValue"), value.toString()}};
	case QMetaType::QByteArray:
		return QCborMap {{QStringLiteral("bytesValue"), QString::fromUtf8(value.toByteArray().toBase64(QByteArray::Base64UrlEncoding | QByteArray::KeepTrailingEquals))}};
	case QMetaType::QVariantList: {
		QCborArray values;
		for (const auto &variant : value.toList())
			values.append(serializeValue(variant));
		return QCborMap {
			{QStringLiteral("arrayValue"), QCborMap {
				{QStringLiteral("values"), values}
			}
		}};
	}
	case QMetaType::QVariantMap:
	case QMetaType::QVariantHash: {
		QCborMap fields;
		const auto vMap = value.toMap();
		for (auto it = vMap.begin(), end = vMap.end(); it != end; ++it)
			fields.insert(it.key(), serializeValue(it.value()));
		return QCborMap {
			{QStringLiteral("mapValue"), QCborMap {
				{QStringLiteral("fields"), fields}
			}
		}};
	}
	default:
		throw QtJsonSerializer::SerializationException {
			QByteArray{"Unable to transform type "} +
			QMetaType::typeName(value.userType()) +
			QByteArray{" to a firestore datatype!"}
		};
	}
}

QVariant DocumentFieldsTypeConverter::deserializeValue(const QCborValue &value) const
{
	if (!value.isMap())
		throw QtJsonSerializer::DeserializationException{"Unexpected value type! Must be a JSON object!"};

	const auto map = value.toMap();
	// find value type
	if (const auto type = QStringLiteral("nullValue"); map.contains(type))
		return QVariant::fromValue(nullptr);
	else if (const auto type = QStringLiteral("booleanValue"); map.contains(type))
		return map.value(type).toBool();
	else if (const auto type = QStringLiteral("integerValue"); map.contains(type))
		return map.value(type).toString().toLongLong();
	else if (const auto type = QStringLiteral("doubleValue"); map.contains(type))
		return map.value(type).toDouble();
	else if (const auto type = QStringLiteral("timestampValue"); map.contains(type))
		return QDateTime::fromString(map.value(type).toString(), Qt::ISODateWithMs);
	else if (const auto type = QStringLiteral("stringValue"); map.contains(type))
		return map.value(type).toString();
	else if (const auto type = QStringLiteral("bytesValue"); map.contains(type))
		return QByteArray::fromBase64(map.value(type).toString().toUtf8(), QByteArray::Base64UrlEncoding);
	else if (const auto type = QStringLiteral("referenceValue"); map.contains(type))
		return map.value(type).toString();  // references are not supported and treated as normal strings
	else if (const auto type = QStringLiteral("geoPointValue"); map.contains(type))
		return map.value(type).toMap().toVariantMap();   // geopoints are not supported, returned as basic variant map instead
	else if (const auto type = QStringLiteral("arrayValue"); map.contains(type)) {
		const auto values = map.value(type).toMap().value(QStringLiteral("values")).toArray();
		QVariantList resValues;
		resValues.reserve(values.size());
		for (const auto &aValue : values)
			resValues.append(deserializeValue(aValue));
		return resValues;
	} else if (const auto type = QStringLiteral("mapValue"); map.contains(type)) {
		const auto values = map.value(type).toMap().value(QStringLiteral("fields")).toMap();
		QVariantMap resValues;
		for (const auto &mValue : values)
			resValues.insert(mValue.first.toString(), deserializeValue(mValue.second));
		return resValues;
	} else
		throw QtJsonSerializer::DeserializationException {"Unable to find a valid firestore type in value!"};
}
