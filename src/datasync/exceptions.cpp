#include "exceptions.h"

using namespace QtDataSync;

SetupException::SetupException(const QString &message, const QString &setupName) :
	_what(message.arg(setupName).toUtf8())
{}

SetupException::SetupException(const SetupException *cloneFrom) :
	_what(cloneFrom->_what)
{}

const char *SetupException::what() const noexcept
{
	return _what.constData();
}

SetupExistsException::SetupExistsException(const QString &setupName) :
	SetupException(QStringLiteral("Failed to create setup! A setup with the name \"%1\" already exists!"), setupName)
{}

SetupExistsException::SetupExistsException(const SetupException *cloneFrom) :
	SetupException(cloneFrom)
{}

void SetupExistsException::raise() const
{
	throw *this;
}

QException *SetupExistsException::clone() const
{
	return new SetupExistsException(this);
}

SetupLockedException::SetupLockedException(const QString &setupName) :
	SetupException(QStringLiteral("Failed to lock the storage directory for setup \"%1\"! Is it already locked by another process?"), setupName)
{}

SetupLockedException::SetupLockedException(const SetupException *cloneFrom) :
	SetupException(cloneFrom)
{}

void SetupLockedException::raise() const
{
	throw *this;
}

QException *SetupLockedException::clone() const
{
	return new SetupLockedException(this);
}

DataSyncException::DataSyncException(const QString &what) :
	QException(),
	_what(what.toUtf8())
{}

DataSyncException::DataSyncException(const char *what) :
	QException(),
	_what(what)
{}

DataSyncException::DataSyncException(const QByteArray &what) :
	QException(),
	_what(what)
{}

const char *DataSyncException::what() const noexcept
{
	return _what.constData();
}

void DataSyncException::raise() const
{
	throw *this;
}

QException *DataSyncException::clone() const
{
	return new DataSyncException(_what);
}
