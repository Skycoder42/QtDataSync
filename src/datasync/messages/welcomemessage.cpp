#include "welcomemessage_p.h"
using namespace QtDataSync;

WelcomeMessage::WelcomeMessage(bool hasChanges) :
	hasChanges(hasChanges)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const WelcomeMessage &message)
{
	stream << message.hasChanges;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, WelcomeMessage &message)
{
	stream.startTransaction();
	stream >> message.hasChanges;
	stream.commitTransaction();
	return stream;
}
