#ifndef TESTDATA_H
#define TESTDATA_H

#include <QObject>

class TestData
{
	Q_GADGET

	Q_PROPERTY(int id MEMBER id USER true)
	Q_PROPERTY(QString text MEMBER text)

public:
	TestData(int id = -1, QString text = {});

	bool operator ==(const TestData &other) const;

	int id;
	QString text;
};

Q_DECLARE_METATYPE(TestData)

#endif // TESTDATA_H
