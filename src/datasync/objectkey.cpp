#include "objectkey.h"
#include <QtCore/QDebug>
#include <QtCore/QCryptographicHash>
using namespace QtDataSync;

QByteArray ObjectKey::hashed() const
{
	QCryptographicHash hash(QCryptographicHash::Sha3_256);
	hash.addData(typeName.toUtf8());
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

uint QtDataSync::qHash(const QtDataSync::ObjectKey &key, uint seed)
{
	return qHash(key.typeName, seed) ^ qHash(key.id, ~seed);
}

QDebug QtDataSync::operator<<(QDebug debug, const ObjectKey &key)
{
	QDebugStateSaver saver{debug};
	debug.nospace().noquote() << '[' << key.typeName << ':' << key.id << ']';
	return debug;
}
