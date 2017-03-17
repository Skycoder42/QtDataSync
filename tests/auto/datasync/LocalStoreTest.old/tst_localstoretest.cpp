#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <asyncdatastore.h>
#include <setup.h>
#include <wsauthenticator.h>

#include <QtJsonSerializer/QJsonSerializer>

#include "testdata.h"

class LocalStoreTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testSaveAndLoad_data();
	void testSaveAndLoad();

	void testLoadAll();
	void testRemove();
	void testListKeys();

private:
	QtDataSync::AsyncDataStore *store;
	QtDataSync::WsAuthenticator *authenticator;
};

void LocalStoreTest::initTestCase()
{
	QJsonSerializer::registerListConverters<TestData>();
	QtDataSync::Setup().create();
	authenticator = QtDataSync::Setup::authenticatorForSetup<QtDataSync::WsAuthenticator>(this);
	authenticator->setRemoteUrl(QStringLiteral("ws://localhost:4242"));
	store = new QtDataSync::AsyncDataStore(this);
}

void LocalStoreTest::cleanupTestCase()
{
	//DEBUG -> let network finish
	QThread::sleep(3);

	store->deleteLater();
	store = nullptr;
}

void LocalStoreTest::testSaveAndLoad_data()
{
	QTest::addColumn<QString>("key");
	QTest::addColumn<TestData>("data");

	QTest::newRow("data0") << QStringLiteral("420")
						   << TestData(420, "data0");
	QTest::newRow("data1") << QStringLiteral("421")
						   << TestData(421, "data1");
	QTest::newRow("data2") << QStringLiteral("422")
						   << TestData(422, "data2");
}

void LocalStoreTest::testSaveAndLoad()
{
	QFETCH(QString, key);
	QFETCH(TestData, data);

	try {
		auto saveFuture = store->save(data);
		saveFuture.waitForFinished();
		auto result = store->load<TestData>(key).result();
		QCOMPARE(result.id, data.id);
		QCOMPARE(result.text, data.text);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void LocalStoreTest::testLoadAll()
{
	try {
		auto data = store->loadAll<TestData>().result();
		QCOMPARE(data.size(), 3);

		QList<TestData> testList = {
			TestData(420, "data0"),
			TestData(421, "data1"),
			TestData(422, "data2")
		};

		foreach(auto testData, data) {
			auto ok = false;
			foreach(auto realData, testList) {
				if(testData.id == realData.id &&
				   testData.text == realData.text) {
					ok = true;
					break;
				}
			}
			QVERIFY2(ok, "TestData not the same");
		}
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void LocalStoreTest::testRemove()
{
	try {
		store->remove<TestData>("421").waitForFinished();
		QCOMPARE(store->count<TestData>().result(), 2);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void LocalStoreTest::testListKeys()
{
	try {
		auto keys = store->keys<TestData>().result();
		QCOMPARE(keys.size(), 2);
		foreach(auto key, keys)
			store->remove<TestData>(key).waitForFinished();
		QCOMPARE(store->count<TestData>().result(), 0);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(LocalStoreTest)

#include "tst_localstoretest.moc"
