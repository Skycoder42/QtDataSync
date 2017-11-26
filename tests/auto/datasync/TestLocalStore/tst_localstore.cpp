#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
#include <QtDataSync/private/localstore_p.h>
using namespace QtDataSync;

class TestLocalStore : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testEmpty();
	void testSave_data();
	void testSave();

private:
	static const QByteArray TypeName;

	LocalStore *store;
};

const QByteArray TestLocalStore::TypeName("TestType");

void TestLocalStore::initTestCase()
{
	try {
		Setup setup;
		TestLib::setup(setup);
		setup.create();

		store = new LocalStore(this);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::cleanupTestCase()
{
	store->deleteLater();
	store = nullptr;
	Setup::removeSetup(DefaultSetup);
}

void TestLocalStore::testEmpty()
{
	try {
		QCOMPARE(store->count(TypeName), 0ull);
		QVERIFY(store->keys(TypeName).isEmpty());
		QVERIFY_EXCEPTION_THROWN(store->load({TypeName, QStringLiteral("id")}), NoDataException);
		QCOMPARE(store->remove({TypeName, QStringLiteral("id")}), false);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestLocalStore::testSave_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<QJsonObject>("data");

	QTest::newRow("data0") << TestLib::generateKey(420)
						   << TestLib::generateDataJson(420);
	QTest::newRow("data1") << TestLib::generateKey(421)
						   << TestLib::generateDataJson(421);
	QTest::newRow("data2") << TestLib::generateKey(422)
						   << TestLib::generateDataJson(422);
}

void TestLocalStore::testSave()
{
	QFETCH(ObjectKey, key);
	QFETCH(QJsonObject, data);

	try {
		store->save(key, data);
		auto res = store->load(key);
		QCOMPARE(res, data);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(TestLocalStore)

#include "tst_localstore.moc"
