#include "objectkey.h"

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

QDataStream &QtDataSync::operator<<(QDataStream &out, const ObjectKey &key)
{
	out << key.typeName
		<< key.id;
	return out;
}

QDataStream &QtDataSync::operator>>(QDataStream &in, ObjectKey &key)
{
	in.startTransaction();
	in >> key.typeName
	   >> key.id;
	in.commitTransaction();
	return in;
}

QDebug QtDataSync::operator<<(QDebug debug, const ObjectKey &key)
{
	QDebugStateSaver saver(debug);
	debug.nospace().noquote() << '[' << key.typeName << ':' << key.id << ']';
	return debug;
}
