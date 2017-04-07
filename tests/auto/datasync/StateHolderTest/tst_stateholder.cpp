#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include "tst.h"
using namespace QtDataSync;

class StateHolderTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testSave_data();
	void testSave();
	void testRemove_data();
	void testRemove();

private:
	MockLocalStore *store;
	MockStateHolder *holder;
	AsyncDataStore *async;
};

void StateHolderTest::initTestCase()
{
#ifdef Q_OS_LINUX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	Setup setup;
	mockSetup(setup);
	store = static_cast<MockLocalStore*>(setup.localStore());
	holder = static_cast<MockStateHolder*>(setup.stateHolder());
	holder->enabled = true;
	setup.create();

	async = new AsyncDataStore(this);
}

void StateHolderTest::cleanupTestCase()
{
	delete async;
	Setup::removeSetup(Setup::DefaultSetup);
}

void StateHolderTest::testSave_data()
{
	QTest::addColumn<StateHolder::ChangeHash>("state");
	QTest::addColumn<TestData>("data");
	QTest::addColumn<StateHolder::ChangeHash>("result");

	QTest::newRow("create") << StateHolder::ChangeHash()
							 << generateData(33)
							 << generateChangeHash(33, 34, StateHolder::Changed);
	QTest::newRow("change") << generateChangeHash(44, 45, StateHolder::Changed)
							 << generateData(44)
							 << generateChangeHash(44, 45, StateHolder::Changed);
	QTest::newRow("changeRemoved") << generateChangeHash(55, 56, StateHolder::Deleted)
								   << generateData(55)
								   << generateChangeHash(55, 56, StateHolder::Changed);
}

void StateHolderTest::testSave()
{
	QFETCH(StateHolder::ChangeHash, state);
	QFETCH(TestData, data);
	QFETCH(StateHolder::ChangeHash, result);

	holder->mutex.lock();
	holder->pseudoState = state;
	holder->mutex.unlock();

	try {
		auto task = async->save<TestData>(data);
		task.waitForFinished();
		QThread::msleep(250);//changes are saved after completing the task

		holder->mutex.lock();
		[&](){//catch return to still unlock
			QCOMPARE(holder->pseudoState, result);
		}();
		holder->mutex.unlock();
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void StateHolderTest::testRemove_data()
{
	QTest::addColumn<DataSet>("data");
	QTest::addColumn<StateHolder::ChangeHash>("state");
	QTest::addColumn<int>("key");
	QTest::addColumn<StateHolder::ChangeHash>("result");

	QTest::newRow("removeUnchanged") << generateDataJson(4, 5)
									 << StateHolder::ChangeHash()
									 << 4
									 << generateChangeHash(4, 5, StateHolder::Deleted);
	QTest::newRow("removeChanged") << generateDataJson(7, 8)
								   << generateChangeHash(7, 8, StateHolder::Changed)
								   << 7
								   << generateChangeHash(7, 8, StateHolder::Deleted);
	QTest::newRow("removeNonExisting") << DataSet()
									   << StateHolder::ChangeHash()
									   << 13
									   << StateHolder::ChangeHash();
}

void StateHolderTest::testRemove()
{
	QFETCH(DataSet, data);
	QFETCH(StateHolder::ChangeHash, state);
	QFETCH(int, key);
	QFETCH(StateHolder::ChangeHash, result);

	holder->mutex.lock();
	holder->pseudoState = state;
	holder->mutex.unlock();

	//set local store -> required since delete checks if actually deleted
	store->mutex.lock();
	store->enabled = true;
	store->pseudoStore = data;
	store->mutex.unlock();

	try {
		auto task = async->remove<TestData>(key);
		task.waitForFinished();
		QThread::msleep(250);

		holder->mutex.lock();
		[&](){//catch return to still unlock
			QCOMPARE(holder->pseudoState, result);
		}();
		holder->mutex.unlock();
	} catch(QException &e) {
		QFAIL(e.what());
	}

	//free local store
	store->mutex.lock();
	store->enabled = false;
	store->pseudoStore.clear();
	store->mutex.unlock();
}

QTEST_MAIN(StateHolderTest)

#include "tst_stateholder.moc"
