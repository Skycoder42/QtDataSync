#ifndef TESTDATA_H
#define TESTDATA_H

#include <QObject>

class TestData
{
	Q_GADGET

	Q_PROPERTY(int id MEMBER id USER true)
	Q_PROPERTY(QString text MEMBER text)

public:
	TestData(int id = 0, QString text = {});

	bool operator ==(const TestData &other) const;

	int id;
	QString text;
};

#endif // TESTDATA_H
