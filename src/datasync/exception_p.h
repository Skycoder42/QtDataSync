#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "qdatasync_global.h"

#include <QtCore/QException>

namespace QtDataSync {

class Q_DATASYNC_EXPORT Exception : public QException
{
public:
	Exception(const QString &what);
	Exception(const QByteArray &what);
	Exception(const char *what);

public:
	const char *what() const noexcept final;

	void raise() const final;
	QException *clone() const final;

private:
	const QByteArray _what;
};

}

#endif // EXCEPTION_H
