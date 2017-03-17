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
	MockStateHolder *holder;
	AsyncDataStore *async;
};

void StateHolderTest::initTestCase()
{
#ifdef Q_OS_UNIX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	Setup setup;
	mockSetup(setup);
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
							 << StateHolder::ChangeHash({{generateKey(33),StateHolder::Changed}});
	QTest::newRow("change") << StateHolder::ChangeHash({{generateKey(44),StateHolder::Changed}})
							 << generateData(44)
							 << StateHolder::ChangeHash({{generateKey(44),StateHolder::Changed}});
	QTest::newRow("changeRemoved") << StateHolder::ChangeHash({{generateKey(55),StateHolder::Deleted}})
								   << generateData(55)
								   << StateHolder::ChangeHash({{generateKey(55),StateHolder::Changed}});
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

		holder->mutex.lock();
		QCOMPARE(holder->pseudoState, result);
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
									 << StateHolder::ChangeHash({{generateKey(4),StateHolder::Deleted}});
	QTest::newRow("removeChanged") << generateDataJson(7, 8)
								   << StateHolder::ChangeHash({{generateKey(7),StateHolder::Changed}})
								   << 7
								   << StateHolder::ChangeHash({{generateKey(7),StateHolder::Deleted}});
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

	try {
		auto task = async->remove<TestData>(key);
		task.waitForFinished();

		holder->mutex.lock();
		QCOMPARE(holder->pseudoState, result);
		holder->mutex.unlock();
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

QTEST_MAIN(StateHolderTest)

#include "tst_stateholder.moc"
