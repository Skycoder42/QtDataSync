#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <testlib.h>

//DIRTY HACK: allow access for test
#define private public
#include <QtDataSync/private/exchangeengine_p.h>
#undef private

#include <QtDataSync/private/changecontroller_p.h>
#include <QtDataSync/private/synchelper_p.h>
#include <QtDataSync/private/setup_p.h>
using namespace QtDataSync;

class TestChangeController : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testChanges_data();
	void testChanges();

	//last test, to avoid problems
	void testChangeTriggers();

private:
	LocalStore *store;
	ChangeController *controller;
};

void TestChangeController::initTestCase()
{
	try {
		TestLib::init();
		Setup setup;
		TestLib::setup(setup);
		setup.create();

		store = new LocalStore(this);
		controller = new ChangeController(DefaultSetup, this);
		controller->initialize({{QStringLiteral("store"), QVariant::fromValue(store)}});
	} catch(QException &e) {
		QFAIL(e.what());
	}
}

void TestChangeController::cleanupTestCase()
{
	controller->finalize();
	delete controller;
	controller = nullptr;
	delete store;
	store = nullptr;
	Setup::removeSetup(DefaultSetup, true);
}

void TestChangeController::testChanges_data()
{
	QTest::addColumn<ObjectKey>("key");
	QTest::addColumn<QJsonObject>("data");
	QTest::addColumn<bool>("isDelete");
	QTest::addColumn<quint64>("version");
	QTest::addColumn<bool>("complete");

	QTest::newRow("change1") << TestLib::generateKey(42)
							 << TestLib::generateDataJson(42)
							 << false
							 << 1ull
							 << false;
	QTest::newRow("change2") << TestLib::generateKey(42)
							 << TestLib::generateDataJson(42, QStringLiteral("hello world"))
							 << false
							 << 2ull
							 << false;
	QTest::newRow("delete3") << TestLib::generateKey(42)
							 << QJsonObject()
							 << true
							 << 3ull
							 << false;
	QTest::newRow("change4") << TestLib::generateKey(42)
							 << TestLib::generateDataJson(42)
							 << false
							 << 4ull
							 << true;
	QTest::newRow("delete5") << TestLib::generateKey(42)
							 << QJsonObject()
							 << true
							 << 5ull
							 << true;
	QTest::newRow("delete0") << TestLib::generateKey(42)
							 << QJsonObject()
							 << true
							 << 0ull
							 << true;
	QTest::newRow("change1") << TestLib::generateKey(42)
							 << TestLib::generateDataJson(42)
							 << false
							 << 1ull
							 << true;
}

void TestChangeController::testChanges()
{
	QFETCH(ObjectKey, key);
	QFETCH(QJsonObject, data);
	QFETCH(bool, isDelete);
	QFETCH(quint64, version);
	QFETCH(bool, complete);

	controller->setUploadingEnabled(false);
	QCoreApplication::processEvents();
	QSignalSpy activeSpy(controller, &ChangeController::uploadingChanged);
	QSignalSpy changeSpy(controller, &ChangeController::uploadChange);
	QSignalSpy errorSpy(controller, &ChangeController::controllerError);

	try {
		//trigger the change
		if(isDelete)
			store->remove(key);
		else
			store->save(key, data);

		//enable changes
		controller->setUploadingEnabled(true);
		if(!errorSpy.isEmpty())
			QFAIL(errorSpy.takeFirst()[0].toString().toUtf8().constData());

		if(version == 0) {
			QVERIFY(!activeSpy.last()[0].toBool());
			QVERIFY(changeSpy.isEmpty());
			QVERIFY(errorSpy.isEmpty());
			return;
		}

		QCOMPARE(changeSpy.size(), 1);
		QVERIFY(activeSpy.last()[0].toBool());
		auto change = changeSpy.takeFirst();
		auto keyHash = change[0].toByteArray();
		auto syncData = SyncHelper::extract(change[1].toByteArray());
		QCOMPARE(std::get<0>(syncData), isDelete);
		QCOMPARE(std::get<1>(syncData), key);
		QCOMPARE(std::get<2>(syncData), version);
		QCOMPARE(std::get<3>(syncData), data);

		if(complete)
			controller->uploadDone(keyHash);
	} catch(QException &e) {
		QFAIL(e.what());
	}

	controller->clearUploads();
}

void TestChangeController::testChangeTriggers()
{
	for(auto i = 0; i < 5; i++) { //wait for the engine to init itself
		QCoreApplication::processEvents();
		QThread::msleep(100);
	}

	//DIRTY HACK: force the engine pass the local controller to anyone asking
	auto engine = SetupPrivate::engine(DefaultSetup);
	auto old = engine->_changeController;
	engine->_changeController = controller;

	//enable changes
	controller->setUploadingEnabled(true);
	QCoreApplication::processEvents();

	QSignalSpy activeSpy(controller, &ChangeController::uploadingChanged);
	QSignalSpy changeSpy(controller, &ChangeController::uploadChange);
	QSignalSpy errorSpy(controller, &ChangeController::controllerError);

	try {
		//clear
		store->reset();
		QVERIFY(changeSpy.isEmpty());
		QVERIFY(errorSpy.isEmpty());

		auto key = TestLib::generateKey(10);
		auto data = TestLib::generateDataJson(10);

		//save data
		store->save(key, data);
		QVERIFY(changeSpy.wait());
		QVERIFY(activeSpy.last()[0].toBool());

		if(!errorSpy.isEmpty())
			QFAIL(errorSpy.takeFirst()[0].toString().toUtf8().constData());

		QCOMPARE(changeSpy.size(), 1);
		auto change = changeSpy.takeFirst();
		auto keyHash = change[0].toByteArray();
		QCOMPARE(change[1].toByteArray(), SyncHelper::combine(key, 1, data));

		controller->uploadDone(keyHash);
		QVERIFY(activeSpy.wait());
		QVERIFY(!activeSpy.last()[0].toBool());
		QVERIFY(changeSpy.isEmpty());
		QVERIFY(errorSpy.isEmpty());

		//now a delete
		store->remove(key);
		QVERIFY(changeSpy.wait());
		QVERIFY(activeSpy.last()[0].toBool());

		if(!errorSpy.isEmpty())
			QFAIL(errorSpy.takeFirst()[0].toString().toUtf8().constData());

		QCOMPARE(changeSpy.size(), 1);
		change = changeSpy.takeFirst();
		keyHash = change[0].toByteArray();
		QCOMPARE(change[1].toByteArray(), SyncHelper::combine(key, 2));

		controller->uploadDone(keyHash);
		QVERIFY(activeSpy.wait());
		QVERIFY(!activeSpy.last()[0].toBool());
		QVERIFY(changeSpy.isEmpty());
		QVERIFY(errorSpy.isEmpty());

		engine->_changeController = old;
	} catch(QException &e) {
		engine->_changeController = old;
		QFAIL(e.what());
	}
}

QTEST_MAIN(TestChangeController)

#include "tst_changecontroller.moc"
