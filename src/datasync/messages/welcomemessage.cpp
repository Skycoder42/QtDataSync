#include "welcomemessage_p.h"
using namespace QtDataSync;

WelcomeMessage::WelcomeMessage(bool hasChanges) :
	SyncMessage(hasChanges ? InitAction : DoneAction)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const WelcomeMessage &message)
{
	stream << (SyncMessage)message;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, WelcomeMessage &message)
{
	stream.startTransaction();
	stream >> (SyncMessage&)message;
	stream.commitTransaction();
	return stream;
}
