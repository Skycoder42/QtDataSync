#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "qdatasync_global.h"

#include <QtCore/qexception.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT SetupException : public QException
{
public:
	const char *what() const noexcept final;

protected:
	SetupException(const QString &message, const QString &setupName);
	SetupException(const SetupException *cloneFrom);

private:
	const QByteArray _what;
};

class Q_DATASYNC_EXPORT SetupExistsException : public SetupException
{
public:
	SetupExistsException(const QString &setupName);

public:
	void raise() const final;
	QException *clone() const final;

private:
	SetupExistsException(const SetupException *cloneFrom);
};

class Q_DATASYNC_EXPORT SetupLockedException : public SetupException
{
public:
	SetupLockedException(const QString &setupName);

public:
	void raise() const final;
	QException *clone() const final;

private:
	SetupLockedException(const SetupException *cloneFrom);
};

class Q_DATASYNC_EXPORT DataSyncException : public QException
{
public:
	DataSyncException(const QString &what);
	DataSyncException(const char *what);

public:
	const char *what() const noexcept final;

	void raise() const final;
	QException *clone() const final;

private:
	DataSyncException(const QByteArray &what);
	const QByteArray _what;
};

}

#endif // EXCEPTIONS_H
