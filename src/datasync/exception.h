#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <QException>

//TODO refactor
namespace QtDataSync {

class Exception : public QException
{
public:
	Exception(const QString &what);
	Exception(const QByteArray &what);
	Exception(const char *what);

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
