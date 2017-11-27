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

	const QString _setupName;
	const QString _message;
};

}

#define QTDATASYNC_EXCEPTION_NAME(x) #x

#endif // EXCEPTION_H
