#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include "tst.h"
using namespace QtDataSync;

class LocalStoreTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();
	void testLoadAll_data();
	void testLoadAll();

private:
	MockLocalStore *store;
	AsyncDataStore *async;
};

void LocalStoreTest::initTestCase()
{
#ifdef Q_OS_UNIX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	Setup setup;
	mockSetup(setup);
	store = static_cast<MockLocalStore*>(setup.localStore());
	store->enabled = true;
	setup.create();

	async = new AsyncDataStore(this);
}

void LocalStoreTest::cleanupTestCase()
{
	delete async;
	Setup::removeSetup(Setup::DefaultSetup);
}

void LocalStoreTest::testLoadAll_data()
{
	QTest::addColumn<DataSet>("data");
	QTest::addColumn<QList<TestData>>("result");
	QTest::addColumn<bool>("shouldFail");

	QTest::newRow("emptyData") << generateDataJson(0, 0)
							   << generateData(0, 0)
							   << false;
	QTest::newRow("simpleData") << generateDataJson(10, 20)
								<< generateData(10, 20)
								<< false;
	QTest::newRow("invalidData") << generateDataJson(0, 0)
								 << generateData(0, 0)
								 << true;
}

void LocalStoreTest::testLoadAll()
{
	QFETCH(DataSet, data);
	QFETCH(QList<TestData>, result);
	QFETCH(bool, shouldFail);

	store->mutex.lock();
	store->pseudoStore = data;
	store->failCount = shouldFail ? 1 : 0;
	store->mutex.unlock();

	try {
		auto task = async->loadAll<TestData>();
		auto res = task.result();
		QVERIFY(!shouldFail);
		QLISTCOMPARE(res, result);
	} catch(QException e) {
		QVERIFY2(shouldFail, e.what());
	}

}

QTEST_MAIN(LocalStoreTest)

#include "tst_localstore.moc"
