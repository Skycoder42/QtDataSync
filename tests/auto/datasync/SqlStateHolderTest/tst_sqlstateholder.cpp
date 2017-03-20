#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include "tst.h"
#include "QtDataSync/private/sqlstateholder_p.h"

using namespace QtDataSync;

class SqlStateHolderTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testMarkChangedAndList_data();
	void testMarkChangedAndList();
	void testResetChanges();
	void testClearChanges();

private:
	SqlStateHolder *holder;
};

void SqlStateHolderTest::initTestCase()
{
#ifdef Q_OS_UNIX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();

	holder = new SqlStateHolder();

	//create setup to "init" both of them, but datasync itself is not used here
	Setup setup;
	mockSetup(setup);
	setup.setStateHolder(holder)
			.create();

	QThread::msleep(500);//wait for setup to complete, because of direct access
}

void SqlStateHolderTest::cleanupTestCase()
{
	Setup::removeSetup(Setup::DefaultSetup);
}

void SqlStateHolderTest::testMarkChangedAndList_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<StateHolder::ChangeState>("state");
	QTest::addColumn<StateHolder::ChangeHash>("result");

	QTest::newRow("unchanged") << generateKey(1)
							   << StateHolder::Unchanged
							   << StateHolder::ChangeHash();
	QTest::newRow("changed") << generateKey(1)
							 << StateHolder::Changed
							 << generateChangeHash(1, 2, StateHolder::Changed);
	QTest::newRow("deleted") << generateKey(1)
							 << StateHolder::Deleted
							 << generateChangeHash(1, 2, StateHolder::Deleted);
	QTest::newRow("unchangedAgain") << generateKey(1)
									<< StateHolder::Unchanged
									<< StateHolder::ChangeHash();
	QTest::newRow("changed2") << generateKey(2)
							  << StateHolder::Changed
							  << generateChangeHash(2, 3, StateHolder::Changed);
	QTest::newRow("changed3") << generateKey(3)
							  << StateHolder::Changed
							  << generateChangeHash(2, 4, StateHolder::Changed);
}

void SqlStateHolderTest::testMarkChangedAndList()
{
	QFETCH(ObjectKey, key);
	QFETCH(StateHolder::ChangeState, state);
	QFETCH(StateHolder::ChangeHash, result);

	holder->markLocalChanged(key, state);
	auto res = holder->listLocalChanges();
	QCOMPARE(res, result);
}

void SqlStateHolderTest::testResetChanges()
{
	//verify previous state
	auto result = generateChangeHash(2, 4, StateHolder::Changed);
	auto res = holder->listLocalChanges();
	QCOMPARE(res, result);

	result = generateChangeHash(30, 40, StateHolder::Changed);
	holder->resetAllChanges(generateDataJson(30, 40).keys());
	res = holder->listLocalChanges();
	QCOMPARE(res, result);
}

void SqlStateHolderTest::testClearChanges()
{
	QVERIFY(!holder->listLocalChanges().isEmpty());
	holder->clearAllChanges();
	QVERIFY(holder->listLocalChanges().isEmpty());
}

QTEST_MAIN(SqlStateHolderTest)

#include "tst_sqlstateholder.moc"
