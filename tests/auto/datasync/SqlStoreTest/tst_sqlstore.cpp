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
	void testLoadInvalid();
	void testSearch_data();
	void testSearch();
	void testRemove_data();
	void testRemove();

	void testLoadAllKeys();
	void testResetStore();

private:
	SqlLocalStore *store;
};

void SqlStoreTest::initTestCase()
{
#ifdef Q_OS_UNIX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	store = new SqlLocalStore();

	//create setup to "init" both of them, but datasync itself is not used here
	Setup setup;
	mockSetup(setup);
	setup.setLocalStore(store)
			.create();

	QThread::msleep(500);//wait for setup to complete, because of direct access
}

void SqlStoreTest::cleanupTestCase()
{
	Setup::removeSetup(Setup::DefaultSetup);
}

void SqlStoreTest::testSaveAndLoad_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<QJsonObject>("data");

	QTest::newRow("data0") << generateKey(420)
						   << generateDataJson(420);
	QTest::newRow("data1") << generateKey(421)
						   << generateDataJson(421);
	QTest::newRow("data2") << generateKey(422)
						   << generateDataJson(422);
}

void SqlStoreTest::testSaveAndLoad()
{
	QFETCH(ObjectKey, key);
	QFETCH(QJsonObject, data);

	QSignalSpy completedSpy(store, &SqlLocalStore::requestCompleted);
	QSignalSpy failedSpy(store, &SqlLocalStore::requestFailed);

	auto id = 1ull;
	store->save(id, key, data, "id");
	QCOMPARE(failedSpy.size(), 0);
	QCOMPARE(completedSpy.size(), 1);
	QCOMPARE(completedSpy[0][0].toULongLong(), id);

	id = 2ull;
	failedSpy.clear();
	completedSpy.clear();
	store->load(id, key, "id");
	QCOMPARE(failedSpy.size(), 0);
	QCOMPARE(completedSpy.size(), 1);
	QCOMPARE(completedSpy[0][0].toULongLong(), id);
	QCOMPARE(completedSpy[0][1].value<QJsonValue>().toObject(), data);
}

void SqlStoreTest::testLoadAll()
{
	QSignalSpy completedSpy(store, &SqlLocalStore::requestCompleted);
	QSignalSpy failedSpy(store, &SqlLocalStore::requestFailed);

	auto keyList = QJsonArray::fromStringList(generateDataKeys(420, 423));
	auto dataList = dataListJson(generateDataJson(420, 423));

	auto id = 1ull;
	store->count(id, "TestData");
	QCOMPARE(failedSpy.size(), 0);
	QCOMPARE(completedSpy.size(), 1);
	QCOMPARE(completedSpy[0][0].toULongLong(), id);
	QCOMPARE(completedSpy[0][1].value<QJsonValue>().toInt(), 3);

	id = 2ull;
	failedSpy.clear();
	completedSpy.clear();
	store->keys(id, "TestData");
	QCOMPARE(failedSpy.size(), 0);
	QCOMPARE(completedSpy.size(), 1);
	QCOMPARE(completedSpy[0][0].toULongLong(), id);
	QLISTCOMPARE(completedSpy[0][1].value<QJsonValue>().toArray().toVariantList(),
				 keyList.toVariantList());

	id = 3ull;
	failedSpy.clear();
	completedSpy.clear();
	store->loadAll(id, "TestData");
	QCOMPARE(failedSpy.size(), 0);
	QCOMPARE(completedSpy.size(), 1);
	QCOMPARE(completedSpy[0][0].toULongLong(), id);
	QLISTCOMPARE(completedSpy[0][1].value<QJsonValue>().toArray().toVariantList(),
			dataList.toVariantList());
}

void SqlStoreTest::testLoadInvalid()
{
	QSignalSpy completedSpy(store, &SqlLocalStore::requestCompleted);
	QSignalSpy failedSpy(store, &SqlLocalStore::requestFailed);

	failedSpy.clear();
	completedSpy.clear();
	store->load(1ull, generateKey(4711), "id");
	QCOMPARE(completedSpy.size(), 0);
	QCOMPARE(failedSpy.size(), 1);
	QCOMPARE(failedSpy[0][0].toULongLong(), 1ull);
}

void SqlStoreTest::testSearch_data()
{
	QTest::addColumn<QString>("query");
	QTest::addColumn<QJsonArray>("data");

	QTest::newRow("*") << QStringLiteral("*")
					   << dataListJson(generateDataJson(420, 423));
	QTest::newRow("422") << QStringLiteral("422")
						 << dataListJson(generateDataJson(422, 423));
	QTest::newRow("4_2*") << QStringLiteral("4_2*")
						  << dataListJson(generateDataJson(422, 423));
	QTest::newRow("4*2*") << QStringLiteral("4*2*")
						  << dataListJson(generateDataJson(420, 423));
	QTest::newRow("2") << QStringLiteral("2")
					   << QJsonArray();
}

void SqlStoreTest::testSearch()
{
	QFETCH(QString, query);
	QFETCH(QJsonArray, data);

	QSignalSpy completedSpy(store, &SqlLocalStore::requestCompleted);
	QSignalSpy failedSpy(store, &SqlLocalStore::requestFailed);

	auto id = 1ull;
	store->search(id, "TestData", query);
	QCOMPARE(failedSpy.size(), 0);
	QCOMPARE(completedSpy.size(), 1);
	QCOMPARE(completedSpy[0][0].toULongLong(), id);
	QLISTCOMPARE(completedSpy[0][1].value<QJsonValue>().toArray().toVariantList(),
				 data.toVariantList());
}

void SqlStoreTest::testRemove_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<bool>("changed");

	QTest::newRow("data0") << generateKey(422)
						   << true;
	QTest::newRow("data_invalid") << generateKey(77)
								  << false;
}

void SqlStoreTest::testRemove()
{
	QFETCH(ObjectKey, key);
	QFETCH(bool, changed);

	QSignalSpy completedSpy(store, &SqlLocalStore::requestCompleted);
	QSignalSpy failedSpy(store, &SqlLocalStore::requestFailed);

	auto id = 1ull;
	store->remove(id, key, "id");
	QCOMPARE(failedSpy.size(), 0);
	QCOMPARE(completedSpy.size(), 1);
	QCOMPARE(completedSpy[0][0].toULongLong(), id);
	QCOMPARE(completedSpy[0][1].value<QJsonValue>().toBool(), changed);
}

void SqlStoreTest::testLoadAllKeys()
{
	ObjectKey extra = {"Baum", QStringLiteral("42")};
	store->save(1ull, extra, QJsonObject(), "");

	QList<ObjectKey> testList;
	testList.append(generateKey(420));
	testList.append(generateKey(421));
	testList.append(extra);

	auto resList = store->loadAllKeys();
	QLISTCOMPARE(resList, testList);
}

void SqlStoreTest::testResetStore()
{
	store->resetStore();

	QSignalSpy completedSpy(store, &SqlLocalStore::requestCompleted);
	QSignalSpy failedSpy(store, &SqlLocalStore::requestFailed);

	auto id = 1ull;
	store->count(id, "TestData");
	QCOMPARE(failedSpy.size(), 0);
	QCOMPARE(completedSpy.size(), 1);
	QCOMPARE(completedSpy[0][0].toULongLong(), id);
	QCOMPARE(completedSpy[0][1].value<QJsonValue>().toInt(), 0);
}

QTEST_MAIN(SqlStoreTest)

#include "tst_sqlstore.moc"
