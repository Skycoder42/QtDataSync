#ifndef QTDATASYNC_EXCEPTIONS_H
#define QTDATASYNC_EXCEPTIONS_H

#include "qtdatasync_global.h"

#include <QtCore/qexception.h>

namespace QtDataSync {

//! Exception throw if Setup::create fails
class Q_DATASYNC_EXPORT SetupException : public QException
{
public:
	//! @inherit{std::exception::what}
	const char *what() const noexcept final;

protected:
	//! Constructor with error message and setup name
	SetupException(const QString &message, const QString &setupName);
	//! Constructor that clones another exception
	SetupException(const SetupException *cloneFrom);

private:
	const QByteArray _what;
};

//! Exception thrown if a setup with the same name already exsits
class Q_DATASYNC_EXPORT SetupExistsException : public SetupException
{
public:
	//! Constructor with setup name
	SetupExistsException(const QString &setupName);

public:
	//! @inherit{QException::raise}
	void raise() const final;
	//! @inherit{QException::clone}
	QException *clone() const final;

private:
	SetupExistsException(const SetupException *cloneFrom);
};

//! Exception thrown if a setups storage directory is locked by another instance
class Q_DATASYNC_EXPORT SetupLockedException : public SetupException
{
public:
	//! Constructor with setup name
	SetupLockedException(const QString &setupName);

public:
	//! @inherit{QException::raise}
	void raise() const final;
	//! @inherit{QException::clone}
	QException *clone() const final;

private:
	SetupLockedException(const SetupException *cloneFrom);
};

//! Exception thrown if something goes wrong when using the async store
class Q_DATASYNC_EXPORT DataSyncException : public QException
{
public:
	//! Constructor with error message
	DataSyncException(const QString &what);
	//! Constructor with error message
	DataSyncException(const char *what);

public:
	//! @inherit{std::exception::what}
	const char *what() const noexcept final;

	//! @inherit{QException::raise}
	void raise() const final;
	//! @inherit{QException::clone}
	QException *clone() const final;

private:
	DataSyncException(const QByteArray &what);
	const QByteArray _what;
};

}

#endif // QTDATASYNC_EXCEPTIONS_H
