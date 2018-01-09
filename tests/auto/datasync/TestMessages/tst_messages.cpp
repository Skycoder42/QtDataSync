#include <QString>
#include <QtTest>
#include <QCoreApplication>

class TestMessages : public QObject
{
	Q_OBJECT

public:
	TestMessages();

private Q_SLOTS:
	void testSerialization_data();
	void testSerialization();
};

TestMessages::TestMessages()
{
}

void TestMessages::testSerialization_data()
{
	QTest::addColumn<QString>("data");
	QTest::newRow("0") << QString();
}

void TestMessages::testSerialization()
{
	QFETCH(QString, data);
	QVERIFY2(true, "Failure");
}

QTEST_MAIN(TestMessages)

#include "tst_messages.moc"
