#include "objectkey.h"
#include <QtCore/QDebug>
using namespace QtDataSync;

bool ObjectKey::isValid() const
{
	return !typeName.isEmpty();
}

QString ObjectKey::toString() const
{
	return QStringLiteral("[%1:%2]").arg(typeName, id);
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
