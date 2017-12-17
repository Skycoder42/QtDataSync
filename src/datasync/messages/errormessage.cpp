#include "errormessage_p.h"

#include <QtCore/QMetaProperty>

using namespace QtDataSync;

ErrorMessage::ErrorMessage(ErrorMessage::ErrorType type, bool canRecover, const QString &message) :
	type(type),
	canRecover(canRecover),
	message(message)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const ErrorMessage &message)
{
	stream << (int)message.type
		   << message.canRecover
		   << message.message;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, ErrorMessage &message)
{
	stream.startTransaction();
	stream >> (int&)message.type
		   >> message.canRecover
		   >> message.message;
	if(QMetaEnum::fromType<ErrorMessage::ErrorType>().valueToKey(message.type))
		stream.commitTransaction();
	else
		stream.abortTransaction();
	return stream;
}

QDebug QtDataSync::operator<<(QDebug debug, const ErrorMessage &message)
{
	QDebugStateSaver saver(debug);
	debug.nospace().noquote() << "Error Message ("
							  << message.type
							  << ", "
							  << (message.canRecover ? "recoverable" : "unrecoverable")
							  << "): "
							  << message.message;
	return debug;
}
