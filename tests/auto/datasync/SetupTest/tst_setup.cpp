#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include "tst.h"
using namespace QtDataSync;

class SetupTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testCreate_data();
	void testCreate();
	void testRemove_data();
	void testRemove();

private:
	QTemporaryDir tDir;
};

void SetupTest::initTestCase()
{
#ifdef Q_OS_UNIX
	Q_ASSERT(qgetenv("LD_PRELOAD").contains("Qt5DataSync"));
#endif

	tst_init();
}

void SetupTest::cleanupTestCase()
{
}

void SetupTest::testCreate_data()
{
	QTest::addColumn<QString>("name");
	QTest::addColumn<QString>("localDir");
	QTest::addColumn<bool>("nameFail");
	QTest::addColumn<bool>("lockFail");

	QTest::newRow("default") << Setup::DefaultSetup
							 << QString()
							 << false
							 << false;
	QTest::newRow("other") << "other"
						   << QString()
						   << false
						   << false;
	QTest::newRow("other2") << "other"
							<< QString()
							<< true
							<< false;
	QTest::newRow("other3") << "other3"
							<< QString()
							<< false
							<< false;

	QTest::newRow("fixedDir1") << "fixedDir1"
							   << tDir.path()
							   << false
							   << false;
	QTest::newRow("fixedDir2") << "fixedDir2"
							   << tDir.path()
							   << false
							   << true;
}

void SetupTest::testCreate()
{
	QFETCH(QString, name);
	QFETCH(QString, localDir);
	QFETCH(bool, nameFail);
	QFETCH(bool, lockFail);

	Setup setup;
	mockSetup(setup, false);
	if(!localDir.isNull())
		setup.setLocalDir(localDir);
	try {
		setup.create(name);
		QVERIFY(!nameFail);
		QVERIFY(!lockFail);
	} catch (SetupExistsException &e) {
		QVERIFY2(nameFail, e.what());
	} catch (SetupLockedException &e) {
		QVERIFY2(lockFail, e.what());
	} catch (QException &e) {
		QFAIL(e.what());
	}
}

void SetupTest::testRemove_data()
{
	QTest::addColumn<QString>("name");
	QTest::addColumn<bool>("wait");
	QTest::addColumn<bool>("recreate");
	QTest::addColumn<bool>("shouldFail");

	QTest::newRow("other") << "other"
						   << false
						   << false
						   << false;
	QTest::newRow("other3") << "other3"
							<< false
							<< true
							<< true;
	QTest::newRow("fixedDir1") << "fixedDir1"
							   << true
							   << true
							   << false;
}

void SetupTest::testRemove()
{
	QFETCH(QString, name);
	QFETCH(bool, wait);
	QFETCH(bool, recreate);
	QFETCH(bool, shouldFail);

	Setup::removeSetup(name, wait);
	if(recreate) {
		try {
			Setup setup;
			mockSetup(setup, false);
			setup.create(name);
			QVERIFY(!shouldFail);
		} catch (SetupExistsException &e) {
			QVERIFY2(shouldFail, e.what());
		}
	}
}

QTEST_MAIN(SetupTest)

#include "tst_setup.moc"
