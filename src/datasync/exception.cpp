#include "exception_p.h"
using namespace QtDataSync;

Exception::Exception(const QString &what) :
	QException(),
	_what(what.toUtf8())
{}

Exception::Exception(const QByteArray &what) :
	QException(),
	_what(what)
{}

Exception::Exception(const char *what) :
	QException(),
	_what(what)
{}

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
