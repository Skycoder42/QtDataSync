#include "welcomemessage_p.h"
using namespace QtDataSync;

WelcomeMessage::WelcomeMessage() {}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const WelcomeMessage &message)
{
	Q_UNUSED(message)
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, WelcomeMessage &message)
{
	Q_UNUSED(message)
	return stream;
}
