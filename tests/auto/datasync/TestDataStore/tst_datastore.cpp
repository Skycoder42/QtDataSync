#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>
using namespace QtDataSync;

class TestDataStore : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

//	void testCount_data();
//	void testCount();
//	void testKeys_data();
//	void testKeys();
//	void testLoadAll_data();
//	void testLoadAll();
//	void testLoad_data();
//	void testLoad();
//	void testSave_data();
//	void testSave();
//	void testRemove_data();
//	void testRemove();
//	void testSearch_data();
//	void testSearch();
//	void testIterate_data();
//	void testIterate();

//	void testLoadInto_data();
//	void testLoadInto();

private:
	DataStore *store;
};

void TestDataStore::initTestCase()
{
	try {
		Setup setup;
		TestLib::setup(setup);
		setup.create();

		store = new DataStore(this);
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestDataStore::cleanupTestCase()
{
	store->deleteLater();
	store = nullptr;
	Setup::removeSetup(DefaultSetup);
}

QTEST_MAIN(TestDataStore)

#include "tst_datastore.moc"
