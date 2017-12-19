#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <QtCore/qexception.h>

#include "qtdatasync_global.h"

namespace QtDataSync {

class Defaults;

class Q_DATASYNC_EXPORT Exception : public QException
{
public:
	Exception(const QString &setupName, const QString &message);
	Exception(const Defaults &defaults, const QString &message);

	virtual QByteArray className() const noexcept;
	QString setupName() const;
	QString message() const;

	virtual QString qWhat() const;
	const char *what() const noexcept final;

	void raise() const override;
	QException *clone() const override;

protected:
	Exception(const Exception * const other);

	QString _setupName;
	QString _message;
};

class Q_DATASYNC_EXPORT SetupDoesNotExistException : public Exception
{
public:
	SetupDoesNotExistException(const QString &setupName);

	QByteArray className() const noexcept override;
	void raise() const override;
	QException *clone() const override;

protected:
	SetupDoesNotExistException(const SetupDoesNotExistException * const other);
};

}

#define __QTDATASYNC_EXCEPTION_NAME_IMPL(x) #x
#define QTDATASYNC_EXCEPTION_NAME(x) "QtDataSync::" __QTDATASYNC_EXCEPTION_NAME_IMPL(x)

#endif // EXCEPTION_H
