#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <asyncdatastore.h>
#include <setup.h>

#include "testdata.h"

class LocalStoreTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testSimpleSave_data();
	void testSimpleSave();

private:
	QtDataSync::AsyncDataStore *store;
};

void LocalStoreTest::initTestCase()
{
	QtDataSync::Setup().create();
	store = new QtDataSync::AsyncDataStore(this);
}

void LocalStoreTest::cleanupTestCase()
{
	store->deleteLater();
	store = nullptr;
}

void LocalStoreTest::testSimpleSave_data()
{
	QTest::addColumn<QString>("key");
	QTest::addColumn<TestData*>("data");

	QTest::newRow("simple") << QStringLiteral("simple")
							<< new TestData(42, this);
}

void LocalStoreTest::testSimpleSave()
{
	QFETCH(QString, key);
	QFETCH(TestData*, data);

	try {
		auto saveFuture = store->save(key, data);
		saveFuture.waitForFinished();
		auto result = store->load<TestData*>(key).result();
		QVERIFY(result);
		QCOMPARE(result->test, data->test);

		result->deleteLater();
	} catch(QException &e) {
		QFAIL(e.what());
	}

	data->deleteLater();
}

QTEST_MAIN(LocalStoreTest)

#include "tst_localstoretest.moc"
