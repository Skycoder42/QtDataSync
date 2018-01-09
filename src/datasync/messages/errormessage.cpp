#include "errormessage_p.h"

#include <QtCore/QMetaEnum>

using namespace QtDataSync;

ErrorMessage::ErrorMessage(ErrorMessage::ErrorType type, const QString &message, bool canRecover) :
	type(type),
	message(message),
	canRecover(canRecover)
{}

const QMetaObject *ErrorMessage::getMetaObject() const
{
	return &staticMetaObject;
}

bool ErrorMessage::validate()
{
	if(!QMetaEnum::fromType<ErrorMessage::ErrorType>().valueToKey(type))
		type = ErrorMessage::UnknownError;
	return true;
}

QDebug QtDataSync::operator<<(QDebug debug, const ErrorMessage &message)
{
	QDebugStateSaver saver(debug);
	debug.nospace().noquote() << "ErrorMessage["
							  << QMetaEnum::fromType<ErrorMessage::ErrorType>().valueToKey(message.type)
							  << ", "
							  << (message.canRecover ? "recoverable" : "unrecoverable")
							  << "]: "
							  << (message.message.isNull() ? QStringLiteral("<no message text>") : static_cast<QString>(message.message));
	return debug;
}
