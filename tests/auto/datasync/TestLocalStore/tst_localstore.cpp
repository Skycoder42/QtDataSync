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

QTEST_MAIN(TestLocalStore)

#include "tst_localstore.moc"
