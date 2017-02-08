#ifndef TESTDATA_H
#define TESTDATA_H

#include <QObject>

class TestData : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int id MEMBER id USER true)
	Q_PROPERTY(QString text MEMBER text)

public:
	Q_INVOKABLE TestData(QObject *parent = nullptr);
	TestData(int id, QString text, QObject *parent = nullptr);

	int id;
	QString text;
};

#endif // TESTDATA_H
