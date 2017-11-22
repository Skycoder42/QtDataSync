#include "exception.h"
#include "defaults.h"
using namespace QtDataSync;

Exception::Exception(const QString &setupName, const QString &message) :
	QException(),
	_setupName(setupName),
	_message(message)
{}

Exception::Exception(const Defaults &defaults, const QString &message) :
	Exception(defaults.setupName(), message)
{}

Exception::Exception(const Exception * const other) :
	QException(),
	_setupName(other->_setupName),
	_message(other->_message)
{}

QString Exception::setupName() const
{
	return _setupName;
}

QString Exception::message() const
{
	return _message;
}

QString Exception::qWhat() const
{
	return QStringLiteral("Message: %1"
						  "\n\tSetup: %2")
			.arg(_message)
			.arg(_setupName);
}

const char *Exception::what() const noexcept
{
	static QByteArray bCache;
	bCache = qWhat().toUtf8();
	return bCache.constData();
}

void Exception::raise() const
{
	throw (*this);
}

QException *Exception::clone() const
{
	return new Exception(this);
}
