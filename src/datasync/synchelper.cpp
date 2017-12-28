#include "synchelper_p.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QVariant>
#include <QtCore/QLocale>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>

#include "message_p.h"


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

QByteArray SyncHelper::combine(const ObjectKey &key, quint64 version, const QJsonObject &data)
{
	QByteArray out;
	QDataStream stream(&out, QIODevice::WriteOnly | QIODevice::Unbuffered);
	setupStream(stream);

	stream << key
		   << version
		   << QJsonDocument(data).toJson(QJsonDocument::Compact);

	if(stream.status() != QDataStream::Ok)
		throw DataStreamException(stream);
	return out;
}

QByteArray SyncHelper::combine(const ObjectKey &key, quint64 version)
{
	QByteArray out;
	QDataStream stream(&out, QIODevice::WriteOnly | QIODevice::Unbuffered);
	setupStream(stream);

	stream << key
		   << version
		   << QByteArray();

	if(stream.status() != QDataStream::Ok)
		throw DataStreamException(stream);
	return out;
}

std::tuple<bool, ObjectKey, quint64, QJsonObject> SyncHelper::extract(const QByteArray &data)
{
	ObjectKey key;
	quint64 version;
	QByteArray jData;

	QDataStream stream(data);
	setupStream(stream);

	stream.startTransaction();
	stream >> key
		   >> version
		   >> jData;

	QJsonObject obj;
	if(jData.isNull())
		stream.commitTransaction();
	else {
		QJsonParseError error;
		auto doc = QJsonDocument::fromJson(jData, &error);
		if(error.error != QJsonParseError::NoError || !doc.isObject())
			stream.abortTransaction();
		else {
			obj = doc.object();
			stream.commitTransaction();
		}
	}

	if(stream.status() != QDataStream::Ok)
		throw DataStreamException(stream);

	return std::tuple<bool, ObjectKey, quint64, QJsonObject>{jData.isNull(), key, version, obj};
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
