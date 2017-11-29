#ifndef TESTOBJECT_H
#define TESTOBJECT_H

#include <QObject>

class TestObject : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int id MEMBER id NOTIFY idChanged USER true)
	Q_PROPERTY(QString text MEMBER text NOTIFY textChanged)

public:
	Q_INVOKABLE TestObject(QObject *parent = nullptr);

	int id;
	QString text;

	bool equals(const TestObject *other) const;

signals:
	void idChanged(int id);
	void textChanged(QString text);
};

#endif // TESTOBJECT_H
