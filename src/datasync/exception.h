#ifndef QTDATASYNC_EXCEPTION_H
#define QTDATASYNC_EXCEPTION_H

#include "QtDataSync/qtdatasync_global.h"

#include <optional>

#include <QtCore/qfiledevice.h>
#include <QtCore/qjsondocument.h>

#if defined(DOXYGEN_RUN) || (!defined(QT_NO_EXCEPTIONS) && QT_CONFIG(future))
#include <QtCore/qexception.h>
namespace QtDataSync {
//! The base class for exceptions of the module
using ExceptionBase = QException;
}
#else
#include <exception>
namespace QtDataSync {
using ExceptionBase = std::exception;
}
#endif

namespace QtDataSync {

class Q_DATASYNC_EXPORT Exception : public ExceptionBase
{
public:
	virtual QString qWhat() const = 0;
	//! @inherit{std::exception::what}
	const char *what() const noexcept final;

	//! @inherit{QException::raise}
	virtual Q_NORETURN void raise() const;
	//! @inherit{QException::clone}
	virtual ExceptionBase *clone() const;

private:
	mutable std::optional<QByteArray> _qWhat;
};

class Q_DATASYNC_EXPORT FileException : public Exception
{
public:
	FileException(const QIODevice *device);
	FileException(const QFileDevice *device);

	QString fileName() const;
	QString errorString() const;

	QString qWhat() const override;
	Q_NORETURN void raise() const override;
	ExceptionBase *clone() const override;

protected:
	QString _fileName;
	QString _errorString;
};

class Q_DATASYNC_EXPORT JsonException : public FileException
{
public:
	JsonException(QJsonParseError error, const QIODevice *device);
	JsonException(QJsonParseError error, const QFileDevice *device);

	int offset() const;
	QJsonParseError::ParseError error() const;

	QString qWhat() const override;
	Q_NORETURN void raise() const override;
	ExceptionBase *clone() const override;

protected:
	QJsonParseError _error;
};

class Q_DATASYNC_EXPORT PListException : public FileException
{
public:
	PListException(const QIODevice *device);

	QString qWhat() const override;
	Q_NORETURN void raise() const override;
	ExceptionBase *clone() const override;
};


}

#endif // QTDATASYNC_EXCEPTION_H
