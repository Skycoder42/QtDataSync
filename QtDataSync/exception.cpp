#include "exception.h"
using namespace QtDataSync;

Exception::Exception(const QString &what) :
	QException(),
	_what(what.toUtf8())
{}

Exception::Exception(const QByteArray &what) :
	QException(),
	_what(what)
{}

QString Exception::qWhat() const
{
	return QString::fromUtf8(_what);
}

const char *Exception::what() const noexcept
{
	return _what.constData();
}

void Exception::raise() const
{
	throw *this;
}

QException *Exception::clone() const
{
	return new Exception(_what);
}
