#include "welcomemessage_p.h"
using namespace QtDataSync;

WelcomeMessage::WelcomeMessage(bool hasChanges) :
	hasChanges(hasChanges),
	keyIndex(0),
	scheme(),
	key(),
	cmac()
{}

bool WelcomeMessage::hasKeyUpdate() const
{
	return !key.isEmpty();
}

QByteArray WelcomeMessage::signatureData(const QUuid &deviceId) const
{
	// keyIndex, scheme, deviceId, key
	return QByteArray::number(keyIndex) +
			scheme +
			deviceId.toRfc4122() +
			key;
}

const QMetaObject *WelcomeMessage::getMetaObject() const
{
	return &staticMetaObject;
}
