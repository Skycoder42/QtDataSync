#include "synchelper_p.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QVariant>
#include <QtCore/QLocale>

using namespace QtDataSync;
using namespace QtDataSync::SyncHelper;

namespace {
void hashNext(QCryptographicHash &hash, const QJsonValue &value);
}

QByteArray SyncHelper::jsonHash(const QJsonObject &object)
{
	QCryptographicHash hash(QCryptographicHash::Sha3_256);
	hashNext(hash, object);
	return hash.result();
}

QJsonObject SyncHelper::combine(const ObjectKey &key, quint64 version, const QJsonObject &data)
{
	QJsonObject res;
	res.insert(QStringLiteral("type"), QString::fromUtf8(key.typeName));
	res.insert(QStringLiteral("id"), key.id);
	res.insert(QStringLiteral("version"), QString::number(version, 36));//encode as string to not loose precision
	res.insert(QStringLiteral("data"), data);
	return res;
}

QJsonObject SyncHelper::combine(const ObjectKey &key, quint64 version)
{
	QJsonObject res;
	res.insert(QStringLiteral("type"), QString::fromUtf8(key.typeName));
	res.insert(QStringLiteral("id"), key.id);
	res.insert(QStringLiteral("version"), QString::number(version, 36));//encode as string to not loose precision
	res.insert(QStringLiteral("data"), QJsonValue(QJsonValue::Null));
	return res;
}

SyncData SyncHelper::extract(const QJsonObject &data)
{
	auto type = data.value(QStringLiteral("type")).toString().toUtf8();
	auto id = data.value(QStringLiteral("id")).toString();
	auto version = data.value(QStringLiteral("version")).toString().toULongLong(nullptr, 36);
	auto jData = data.value(QStringLiteral("data"));

	return std::make_tuple(jData.isNull(), ObjectKey{type, id}, version, jData.toObject());
}

namespace {

void hashNext(QCryptographicHash &hash, const QJsonValue &value)
{
	switch (value.type()) {
	case QJsonValue::Null:
		hash.addData("null");
		break;
	case QJsonValue::Bool:
		hash.addData(value.toBool() ? "true" : "false");
		break;
	case QJsonValue::Double:
		hash.addData(QByteArray::number(value.toDouble(), 'g', QLocale::FloatingPointShortest));
		break;
	case QJsonValue::String:
		hash.addData(value.toString().toUtf8());
		break;
	case QJsonValue::Array:
		foreach(auto v, value.toArray())
			hashNext(hash, v);
		break;
	case QJsonValue::Object:
	{
		auto obj = value.toObject();
		for(auto it = obj.begin(); it != obj.end(); it++) { //if "keys" is sorted, this must be as well
			hash.addData(it.key().toUtf8());
			hashNext(hash, it.value());
		}
		break;
	}
	case QJsonValue::Undefined: //ignored for the hash
		break;
	default:
		Q_UNREACHABLE();
		break;
	}
}

}
