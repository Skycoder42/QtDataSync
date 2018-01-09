#include "objectkey.h"
#include "message_p.h"

#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QCryptographicHash>

using namespace QtDataSync;

ObjectKey::ObjectKey(const QByteArray &typeName, const QString &id) :
	typeName(typeName),
	id(id)
{}

QByteArray ObjectKey::hashed() const
{
	QCryptographicHash hash(QCryptographicHash::Sha3_256);
	hash.addData(typeName);
	hash.addData(id.toUtf8());
	return hash.result();
}

bool ObjectKey::operator==(const QtDataSync::ObjectKey &other) const
{
	return typeName == other.typeName &&
		id == other.id;
}

bool ObjectKey::operator!=(const QtDataSync::ObjectKey &other) const
{
	return typeName != other.typeName ||
		id != other.id;
}

uint QtDataSync::qHash(const QtDataSync::ObjectKey &key, uint seed) {
	return qHash(key.typeName, seed) ^ qHash(key.id, seed);
}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const ObjectKey &key)
{
	stream << key.typeName
		   << static_cast<Utf8String>(key.id);
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, ObjectKey &key)
{
	Utf8String id;
	stream >> key.typeName
		   >> id;
	key.id = id;
	return stream;
}

QDebug QtDataSync::operator<<(QDebug debug, const ObjectKey &key)
{
	QDebugStateSaver saver(debug);
	debug.nospace().noquote() << '[' << key.typeName << ':' << key.id << ']';
	return debug;
}
