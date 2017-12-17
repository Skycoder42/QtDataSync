#include "errormessage_p.h"

#include <QtCore/QMetaEnum>

using namespace QtDataSync;

ErrorMessage::ErrorMessage(ErrorMessage::ErrorType type, const QString &message, bool canRecover) :
	type(type),
	message(message),
	canRecover(canRecover)
{}

QDataStream &QtDataSync::operator<<(QDataStream &stream, const ErrorMessage &message)
{
	stream << (int)message.type
		   << message.message
		   << message.canRecover;
	return stream;
}

QDataStream &QtDataSync::operator>>(QDataStream &stream, ErrorMessage &message)
{
	stream.startTransaction();
	stream >> (int&)message.type
		   >> message.message
		   >> message.canRecover;
	if(!QMetaEnum::fromType<ErrorMessage::ErrorType>().valueToKey(message.type))
		message.type = ErrorMessage::UnknownError;
	stream.commitTransaction();
	return stream;
}

QDebug QtDataSync::operator<<(QDebug debug, const ErrorMessage &message)
{
	QDebugStateSaver saver(debug);
	debug.nospace().noquote() << "ErrorMessage["
							  << QMetaEnum::fromType<ErrorMessage::ErrorType>().valueToKey(message.type)
							  << ", "
							  << (message.canRecover ? "recoverable" : "unrecoverable")
							  << "]: "
							  << (message.message.isNull() ? QStringLiteral("<no message text>") : message.message);
	return debug;
}
