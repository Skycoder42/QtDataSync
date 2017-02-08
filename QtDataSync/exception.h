#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <QException>

namespace QtDataSync {

class Exception : public QException
{
public:
	explicit Exception(const QString &what);
	Exception(const QByteArray &what);

public:
	QString qWhat() const;
	const char *what() const noexcept final;

	void raise() const final;
	QException *clone() const final;

private:
	const QByteArray _what;
};

}

#endif // EXCEPTION_H
