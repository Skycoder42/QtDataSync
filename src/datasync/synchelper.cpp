#include "synchelper_p.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QVariant>
#include <QtCore/QLocale>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>

#include "message_p.h"

using namespace QtDataSync;
using namespace QtDataSync::SyncHelper;
using std::tuple;
using std::make_tuple;

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
	Message::setupStream(stream);

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
	Message::setupStream(stream);

	stream << key
		   << version
		   << QByteArray();

	if(stream.status() != QDataStream::Ok)
		throw DataStreamException(stream);
	return out;
}

tuple<bool, ObjectKey, quint64, QJsonObject> SyncHelper::extract(const QByteArray &data)
{
	ObjectKey key;
	quint64 version;
	QByteArray jData;

	QDataStream stream(data);
	Message::setupStream(stream);

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

	return make_tuple(jData.isNull(), key, version, obj);
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
		for(auto v : value.toArray())
			hashNext(hash, v);
		break;
	case QJsonValue::Object:
	{
		auto obj = value.toObject();
		//helper code to assert the obj iterator is sorted.
#ifndef QT_NO_DEBUG
		QString pKey;
#endif
		for(auto it = obj.begin(); it != obj.end(); it++) { //if "keys" is sorted, this must be as well
#ifndef QT_NO_DEBUG
			if(!pKey.isNull())
				Q_ASSERT(pKey < it.key());
			pKey = it.key();
#endif
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
