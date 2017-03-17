#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include "tst.h"
#include "QtDataSync/private/sqllocalstore_p.h"

using namespace QtDataSync;

class SqlStoreTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testSaveAndLoad_data();
	void testSaveAndLoad();

	void testLoadAll();
	void testSeach_data();
	void testSeach();
	void testRemove();
	void testListKeys();

private:
	QtDataSync::AsyncDataStore *async;
};

void SqlStoreTest::initTestCase()
{
#ifdef Q_OS_UNIX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	Setup setup;
	mockSetup(setup);
	setup.setLocalStore(new SqlLocalStore())
			.create();

	async = new AsyncDataStore(this);
}

void SqlStoreTest::cleanupTestCase()
{
	delete async;
}

void SqlStoreTest::testSaveAndLoad_data()
{
	QTest::addColumn<QString>("key");
	QTest::addColumn<TestData>("data");

	QTest::newRow("data0") << QStringLiteral("420")
						   << generateData(420);
	QTest::newRow("data1") << QStringLiteral("421")
						   << generateData(421);
	QTest::newRow("data2") << QStringLiteral("422")
						   << generateData(422);
}

void SqlStoreTest::testSaveAndLoad()
{
	QFETCH(QString, key);
	QFETCH(TestData, data);

	try {
		auto saveFuture = async->save<TestData>(data);
		saveFuture.waitForFinished();
		auto result = async->load<TestData>(key).result();
		QCOMPARE(result, data);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void SqlStoreTest::testLoadAll()
{
	auto testList = generateData(420, 423);
	try {
		auto data = async->loadAll<TestData>().result();
		QLISTCOMPARE(data, testList);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void SqlStoreTest::testSeach_data()
{
	QTest::addColumn<QString>("query");
	QTest::addColumn<QList<TestData>>("data");

	QTest::newRow("*") << QStringLiteral("*")
					   << generateData(420, 423);
	QTest::newRow("422") << QStringLiteral("422")
						 << generateData(422, 423);
	QTest::newRow("4_2*") << QStringLiteral("4_2*")
						  << generateData(422, 423);
	QTest::newRow("4*2*") << QStringLiteral("4*2*")
						  << generateData(420, 423);
	QTest::newRow("baum") << QStringLiteral("baum")
						  << QList<TestData>();
}

void SqlStoreTest::testSeach()
{
	QFETCH(QString, query);
	QFETCH(QList<TestData>, data);

	try {
		auto res = async->search<TestData>(query).result();
		QLISTCOMPARE(res, data);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void SqlStoreTest::testRemove()
{
	try {
		async->remove<TestData>(QStringLiteral("422")).waitForFinished();
		QCOMPARE(async->count<TestData>().result(), 2);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void SqlStoreTest::testListKeys()
{
	auto testList = generateDataKeys(420, 422);
	try {
		auto keys = async->keys<TestData>().result();
		QLISTCOMPARE(keys, testList);
		foreach(auto key, keys)
			async->remove<TestData>(key).waitForFinished();
		QCOMPARE(async->count<TestData>().result(), 0);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(SqlStoreTest)

#include "tst_sqlstore.moc"
