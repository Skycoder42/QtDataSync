#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <asyncdatastore.h>

class LocalStoreTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();
	void testLifeCycle();

private:
	QtDataSync::AsyncDataStore *store;
};

void LocalStoreTest::initTestCase()
{
	store = new QtDataSync::AsyncDataStore(this);
}

void LocalStoreTest::cleanupTestCase()
{
}

void LocalStoreTest::testLifeCycle()
{
	QVERIFY2(true, "Failure");
}

QTEST_MAIN(LocalStoreTest)

#include "tst_localstoretest.moc"
