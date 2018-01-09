#include "welcomemessage_p.h"
using namespace QtDataSync;

WelcomeMessage::WelcomeMessage(bool hasChanges, const QList<KeyUpdate> &keyUpdates) :
	hasChanges(hasChanges),
	keyUpdates(keyUpdates)
{}

QByteArray WelcomeMessage::signatureData(const QUuid &deviceId, const WelcomeMessage::KeyUpdate &deviceInfo)
{
	// keyIndex, scheme, deviceId, key
	return QByteArray::number(std::get<0>(deviceInfo)) +
			std::get<1>(deviceInfo) +
			deviceId.toRfc4122() +
			std::get<2>(deviceInfo);
}

const QMetaObject *WelcomeMessage::getMetaObject() const
{
	return &staticMetaObject;
}
