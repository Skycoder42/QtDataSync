#ifndef QTDATASYNC_EXCEPTION_H
#define QTDATASYNC_EXCEPTION_H

#include <QtCore/qexception.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

class Defaults;

//! The base class for all exceptions of QtDataSync
class Q_DATASYNC_EXPORT Exception : public QException
{
public:
	//! Constructor for an exception from the given setup
	Exception(const QString &setupName, const QString &message);
	//! Constructor for an exception with the given defaults
	Exception(const Defaults &defaults, const QString &message);

	//! Returns the name of the exception
	virtual QByteArray className() const noexcept;
	//! Returns the name of the setup the exception was thrown from
	QString setupName() const;
	//! Returns the actual error message of the exception
	QString message() const;

	//! Like what, but returns a QString instead of a character array
	virtual QString qWhat() const;
	//! @inherit{QException::what}
	const char *what() const noexcept final; //mark virtual again for doxygen

	//! @inherit{QException::raise}
	virtual void raise() const override; //mark virtual again for doxygen
	//! @inherit{QException::clone}
	virtual QException *clone() const override; //mark virtual again for doxygen

protected:
	//! @private
	Exception(const Exception * const other);

	//! @private
	QString _setupName;
	//! @private
	QString _message;
	//! @private
	mutable QByteArray _qWhat;
};

//! An Exception thrown in case a class is trying to access a setup that does not exist
class Q_DATASYNC_EXPORT SetupDoesNotExistException : public Exception
{
public:
	//! Constructor for an exception from the given setup
	SetupDoesNotExistException(const QString &setupName);

	QByteArray className() const noexcept override;
	void raise() const override;
	QException *clone() const override;

protected:
	//! @private
	SetupDoesNotExistException(const SetupDoesNotExistException * const other);
};

}

//! @private
#define __QTDATASYNC_EXCEPTION_NAME_IMPL(x) #x
//! @private
#define QTDATASYNC_EXCEPTION_NAME(x) "QtDataSync::" __QTDATASYNC_EXCEPTION_NAME_IMPL(x)

#endif // QTDATASYNC_EXCEPTION_H
